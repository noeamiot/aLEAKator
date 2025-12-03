#include "lss.h"
#include <chrono>
#include <fstream>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "configuration.h"

// Configuration constructor. Formats options and create relevant initial directories and files
Configuration::Configuration(int argc, char *argv[], CircuitType circuit_type, const std::string& program) : circuit_type_(circuit_type), program_(program) {
    // Use boost for parameters handling
    po::options_description desc(std::string(argv[0]) + " options");
    desc.add_options()
        ("subprogram", po::value<std::string>(), "subprogram to run on CPU (CPU only and positional)")
        ("help", "produce help message")
        ("twg",  po::value<bool>()->default_value(this->VERIF_TRANSITION_W_GLITCHES_)->implicit_value(true), "Verify Transitions With Glitches")
        ("twog", po::value<bool>()->default_value(this->VERIF_TRANSITION_WO_GLITCHES_)->implicit_value(true), "Verify Transitions Without Glitches")
        ("vwg",  po::value<bool>()->default_value(this->VERIF_VALUE_W_GLITCHES_)->implicit_value(true), "Verify Values With Glitches")
        ("vwog", po::value<bool>()->default_value(this->VERIF_VALUE_WO_GLITCHES_)->implicit_value(true), "Verify Values Without Glitches")
        ("bit-verif", po::value<bool>()->default_value(circuit_type_ != CircuitType::CPU)->implicit_value(true), "if true verification is size is bit, otherwise, it is support")
        ("skip", po::value<unsigned int>()->default_value(this->SKIP_VERIF_CYCLES_), "Skip X cycles before starting verif")
        ("dbg", po::value<bool>()->default_value(false)->implicit_value(true), "If true, do not redirect output to file")
        ("force", po::value<bool>()->default_value(this->FORCE_VERIFY_ALL_)->implicit_value(true), "Force verification of all wires when glitches are considered")
        ("detailed", po::value<bool>()->default_value(this->DETAIL_LEAKS_INFORMATION_)->implicit_value(true), "Give full information about position in circuit about identified leaks")
        ("show-expr", po::value<bool>()->default_value(this->DETAIL_SHOW_EXPRESSION_)->implicit_value(true), "Display the expressions (be carefull, they may be too big to print) for identified leaks")
        ("track", po::value<bool>()->default_value(this->TRACK_LEAKS_)->implicit_value(true), "Tracks leakage up to the root of the leakage")
        ("ho-spatial", po::value<bool>()->default_value(false)->implicit_value(true), "Higher order means spatial for you. This is the default.")
        ("ho-temporal", po::value<bool>()->default_value(false)->implicit_value(true), "Higher order means temporal for you")
        ("order", po::value<size_t>()->default_value(this->ORDER_VERIF_), "Order of verification to perform.")
        ("property", po::value<std::string>()->default_value("TPS"), "Security property to verify.")
    ;

    // Only for CPUs, take a subprogram as option. It is positional
    po::variables_map vm;
    if (circuit_type_ == CircuitType::CPU) {
        po::positional_options_description p;
        p.add("subprogram", 1);
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    } else {
        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
    }
    po::notify(vm);

    // If user asks for help or did not provide a subprogram in case of CPU, display help
    if (vm.count("help") or (circuit_type_ == CircuitType::CPU and not vm.count("subprogram"))) {
        std::cout << desc << "\n";
        std::exit(EXIT_SUCCESS);
    }

    // Now handle arguments
    if (circuit_type_ == CircuitType::CPU) {
        if (not vm.count("subprogram"))
            throw std::invalid_argument( "Not enough arguments given to program." );
        subprogram_ = vm["subprogram"].as<std::string>();
    }
    this->VERIF_VALUE_W_GLITCHES_ = vm["vwg"].as<bool>();
    this->VERIF_VALUE_WO_GLITCHES_ = vm["vwog"].as<bool>();
    this->VERIF_TRANSITION_W_GLITCHES_ = vm["twg"].as<bool>();
    this->VERIF_TRANSITION_WO_GLITCHES_ = vm["twog"].as<bool>();
    this->BIT_VERIF_ = vm["bit-verif"].as<bool>();
    this->SKIP_VERIF_CYCLES_ = vm["skip"].as<unsigned int>();
    this->FORCE_VERIFY_ALL_ = vm["force"].as<bool>();
    this->DETAIL_LEAKS_INFORMATION_ = vm["detailed"].as<bool>();
    this->DETAIL_SHOW_EXPRESSION_ = vm["show-expr"].as<bool>();
    this->TRACK_LEAKS_ = vm["track"].as<bool>();
    this->ORDER_VERIF_ = vm["order"].as<size_t>();

    if (vm["ho-spatial"].as<bool>() and vm["ho-temporal"].as<bool>())
        throw std::invalid_argument( "ho-spatial and ho-temporal are mutually exclusive." );

    if (this->ORDER_VERIF_ > 1 and (this->VERIF_TRANSITION_WO_GLITCHES_ or this->VERIF_TRANSITION_W_GLITCHES_))
        throw std::invalid_argument( "Transitions with and without glitches are a subset of higher order verification, thus are not implemented here. " );

    std::map<std::string, leaks::Properties> property_map {
        {"TPS", leaks::Properties::TPS},
        {"NI", leaks::Properties::NI},
        {"SNI", leaks::Properties::SNI},
    };

    if (not property_map.contains(vm["property"].as<std::string>()))
        throw std::invalid_argument( "Invalid property, must be one of TPS and NI." );
    this->SECURITY_PROPERTY_ = property_map.at(vm["property"].as<std::string>());
    if (circuit_type_ != GADGET and (this->SECURITY_PROPERTY_ == leaks::SNI or this->SECURITY_PROPERTY_ == leaks::NI))
        throw std::invalid_argument( "NI and SNI are only supported on gadgets." );

    if (this->REMOVE_FALSE_NEGATIVE_ and this->SECURITY_PROPERTY_ != leaks::TPS)
        throw std::invalid_argument( "VerifMSI enumeration is only defined for TPS." );

    // Still, default is spatial
    if (vm["ho-temporal"].as<bool>())
        this->HIGHER_ORDER_TYPE_ = TEMPORAL;
    else
        this->HIGHER_ORDER_TYPE_ = SPATIAL;

    if (this->SECURITY_PROPERTY_ == leaks::SNI and (this->VERIF_TRANSITION_W_GLITCHES_ or this->VERIF_TRANSITION_WO_GLITCHES_ or (this->ORDER_VERIF_ > 1 and HIGHER_ORDER_TYPE_ == TEMPORAL)))
        throw std::invalid_argument( "Transitions are not defined for SNI verification." );

    this->init_working_path(argv[0]);

    // Check that config is coherent
    assert((this->ORDER_VERIF_ >= 1) && "Verification order should be superior or equal to one");
    assert((this->SKIP_VERIF_CYCLES_ >= 0) && "Skip verif cycle should be superior or equal to zero");
    assert((not this->EXIT_AT_FIRST_LEAK_ or this->EXIT_AT_FIRST_LEAKING_CYCLE_) && "If exit at first leak is set, exit at first leaking cycle should be set");
}

// Determine working path, create folder structure and copy program files
void Configuration::init_working_path(const std::string& arg0) {
    fs::path binary_path = fs::weakly_canonical(fs::path(arg0)).parent_path();
    // Get current localed and formatted time
    const std::string time = std::format("{:%Y_%m_%d__%H_%M_%S}", std::chrono::floor<std::chrono::seconds>(
        std::chrono::current_zone()->to_local(std::chrono::system_clock::now())
    ));
    working_path_ = binary_path/"leak_data"/(program_ + "_" + subprogram_ + "_" + time);

    // If folder already exist (the program was already started the same second), recreate it
    if(fs::exists(working_path_)) {
        std::cout << working_path_ << " folder already exists, clearing it !" << std::endl;
        fs::remove_all(working_path_);
    }
    fs::create_directories(working_path_);

    // For CPU case, copy program source and binaries
    if (circuit_type_ == CircuitType::CPU) {
        if (not fs::exists(binary_path/"programs"/(subprogram_ + ".o")))
            throw std::invalid_argument( "Program object file not found." );
        fs::copy_file(binary_path/"programs"/(subprogram_ + ".o"), working_path_/"program.o");

        if (fs::exists(binary_path/"programs"/(subprogram_ + ".c")))
            fs::copy_file(binary_path/"programs"/(subprogram_ + ".c"), working_path_/"program.c");
        if (fs::exists(binary_path/"programs"/(subprogram_ + ".S")))
            fs::copy_file(binary_path/"programs"/(subprogram_ + ".S"), working_path_/"program.S");
    }
}

// Dump configuration to a file "config.txt" in working path
void Configuration::dump() const {
    std::ofstream ofs(working_path_/"config.txt");
    ofs << *this;
    ofs.close();
}

// Print configuration object to stream
std::ostream& operator<<(std::ostream& os, Configuration const& m) {
    os << "Configuration:" << std::endl;
    os << "Working directory: " << m.working_path_ << std::endl;

    os << std::boolalpha;
    os << "BIT_VERIF:" << m.BIT_VERIF_ << std::endl;
    os << "USE_STABILITY:" << m.USE_STABILITY_ << std::endl;
    os << "REMOVE_FALSE_NEGATIVE:" << m.REMOVE_FALSE_NEGATIVE_ << std::endl;
    os << "EXIT_AT_FIRST_LEAKING_CYCLE:" << m.EXIT_AT_FIRST_LEAKING_CYCLE_ << std::endl;
    os << "EXIT_AT_FIRST_LEAK:" << m.EXIT_AT_FIRST_LEAK_ << std::endl;
    os << "VERIF_VALUE_WO_GLITCHES:" << m.VERIF_VALUE_WO_GLITCHES_ << std::endl;
    os << "VERIF_VALUE_W_GLITCHES:" << m.VERIF_VALUE_W_GLITCHES_ << std::endl;
    os << "VERIF_TRANSITION_WO_GLITCHES:" << m.VERIF_TRANSITION_WO_GLITCHES_ << std::endl;
    os << "VERIF_TRANSITION_W_GLITCHES:" << m.VERIF_TRANSITION_W_GLITCHES_ << std::endl;
    os << "TRANSITION_W_GLITCHES_OVER_APPROX:" << m.TRANSITION_W_GLITCHES_OVER_APPROX_ << std::endl;
    os << "FORCE_VERIFY_ALL:" << m.FORCE_VERIFY_ALL_ << std::endl;
    os << "DETAIL_LEAKS_INFORMATION:" << m.DETAIL_LEAKS_INFORMATION_ << std::endl;
    os << "DETAIL_SHOW_EXPRESSION:" << m.DETAIL_SHOW_EXPRESSION_ << std::endl;
    os << "TRACK_LEAKS_:" << m.TRACK_LEAKS_ << std::endl;
    os << std::noboolalpha;

    os << "SECURITY_PROPERTY:" << m.SECURITY_PROPERTY_ << std::endl;
    os << "ORDER_VERIF:" << m.ORDER_VERIF_ << std::endl;
    os << "HIGHER_ORDER_TYPE:" << (m.HIGHER_ORDER_TYPE_ == Configuration::TEMPORAL ? "TEMPORAL" : "SPATIAL") << std::endl;
    os << "SKIP_VERIF_CYCLES:" << m.SKIP_VERIF_CYCLES_ << std::endl;
    os << "CYCLES_TO_VERIFY:" << m.CYCLES_TO_VERIFY_ << std::endl;
    if (not m.EXCEPTIONS_WORD_VERIF_.empty()) {
        os << "EXCEPTIONS_WORD_VERIF:" << std::endl;
        for (const auto &[wire, width] : m.EXCEPTIONS_WORD_VERIF_) {
            os << "- " << wire << ": " << width << std::endl;
        }
    }

    // Dump verifMSI Config to file as well
    dumpConfig(os);
    return os;
}
