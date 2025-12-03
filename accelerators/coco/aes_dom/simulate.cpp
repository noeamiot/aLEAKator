#include <iostream>

#include "verif_msi_pp.hpp"
#include "cxxrtl_aes_dom.h"
#include "manager.h"

bool prepare_step(Manager& manager, cxxrtl_design::p_top& top) {
    top.prev_p_ClkxCI.set<bool, true>(false);
    top.p_ClkxCI.set<bool, true>(true);
    top.p_StartxSI.set_fully_stable();
    top.p_RstxBI.set_fully_stable();
    #ifdef CONST_EXEC
    Node* rZmul1xDI = &constant(0xa, 4);
    Node* rZmul2xDI = &constant(0xb, 4);
    Node* rZmul3xDI = &constant(0xc, 4);
    Node* rZinv1xDI = &constant(3, 2);
    Node* rZinv2xDI = &constant(2, 2);
    Node* rZinv3xDI = &constant(1, 2);
    Node* rBmul1xDI = &constant(0xde, 8);
    Node* rBinv1xDI = &constant(0xf, 4);
    Node* rBinv2xDI = &constant(0x1, 4);
    Node* rBinv3xDI = &constant(0x3, 4);
    top.p_Zmul1xDI.set<uint64_t, true>(rZmul1xDI->cst[0]);
    top.p_Zmul2xDI.set<uint64_t, true>(rZmul2xDI->cst[0]);
    top.p_Zmul3xDI.set<uint64_t, true>(rZmul3xDI->cst[0]);
    top.p_Zinv1xDI.set<uint64_t, true>(rZinv1xDI->cst[0]);
    top.p_Zinv2xDI.set<uint64_t, true>(rZinv2xDI->cst[0]);
    top.p_Zinv3xDI.set<uint64_t, true>(rZinv3xDI->cst[0]);
    top.p_Bmul1xDI.set<uint64_t, true>(rBmul1xDI->cst[0]);
    top.p_Binv1xDI.set<uint64_t, true>(rBinv1xDI->cst[0]);
    top.p_Binv2xDI.set<uint64_t, true>(rBinv2xDI->cst[0]);
    top.p_Binv3xDI.set<uint64_t, true>(rBinv3xDI->cst[0]);
    #else
    unsigned int steps = manager.get_steps();
    if (steps > 1) {
        Node* rZmul1xDI = &symbol(("Zmul1xDI_s" + std::to_string(steps)).c_str(), 'M', 4);
        Node* rZmul2xDI = &symbol(("Zmul2xDI_s" + std::to_string(steps)).c_str(), 'M', 4);
        Node* rZmul3xDI = &symbol(("Zmul3xDI_s" + std::to_string(steps)).c_str(), 'M', 4);
        Node* rZinv1xDI = &symbol(("Zinv1xDI_s" + std::to_string(steps)).c_str(), 'M', 2);
        Node* rZinv2xDI = &symbol(("Zinv2xDI_s" + std::to_string(steps)).c_str(), 'M', 2);
        Node* rZinv3xDI = &symbol(("Zinv3xDI_s" + std::to_string(steps)).c_str(), 'M', 2);
        Node* rBmul1xDI = &symbol(("Bmul1xDI_s" + std::to_string(steps)).c_str(), 'M', 8);
        Node* rBinv1xDI = &symbol(("Binv1xDI_s" + std::to_string(steps)).c_str(), 'M', 4);
        Node* rBinv2xDI = &symbol(("Binv2xDI_s" + std::to_string(steps)).c_str(), 'M', 4);
        Node* rBinv3xDI = &symbol(("Binv3xDI_s" + std::to_string(steps)).c_str(), 'M', 4);
        top.p_Zmul1xDI.setNode(rZmul1xDI);
        top.p_Zmul2xDI.setNode(rZmul2xDI);
        top.p_Zmul3xDI.setNode(rZmul3xDI);
        top.p_Zinv1xDI.setNode(rZinv1xDI);
        top.p_Zinv2xDI.setNode(rZinv2xDI);
        top.p_Zinv3xDI.setNode(rZinv3xDI);
        top.p_Bmul1xDI.setNode(rBmul1xDI);
        top.p_Binv1xDI.setNode(rBinv1xDI);
        top.p_Binv2xDI.setNode(rBinv2xDI);
        top.p_Binv3xDI.setNode(rBinv3xDI);
    }
    #endif

    return manager.step(top);
}

int main(int argc, char *argv[]) {
    Configuration config(argc, argv, Configuration::CircuitType::ACCELERATOR, "aes_dom");
    cxxrtl_design::p_top top;
    Manager manager(top, config);

/*
    uint8_t key[16] = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t  pt[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t ect[16] = {0x0e, 0xdd, 0x33, 0xd3, 0xc6, 0x21, 0xe5, 0x46, 
                       0x45, 0x5b, 0xd8, 0xba, 0x14, 0x18, 0xbe, 0xc8};
*/
/*
    uint8_t key[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
                       0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

    uint8_t  pt[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 
                       0xff, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t ect[16] = {0xf2, 0x56, 0xf2, 0xac, 0x8f, 0xb1, 0xdf, 0xbc, 
                       0x7b, 0xdb, 0xa5, 0x60, 0xbc, 0x64, 0x30, 0xe5};
*/
///*
    uint8_t key[16] = {0x3d, 0xc6, 0xa6, 0xa4, 0x30, 0x19, 0xb1, 0xa4, 
                       0xd5, 0x31, 0x0f, 0x1f, 0x00, 0x4d, 0x8d, 0x2d};
    uint8_t  pt[16] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
                       0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66};
    uint8_t ect[16] = {0x68, 0x4d, 0x58, 0x3b, 0xa3, 0x24, 0x41, 0x6f, 
                       0xad, 0xe4, 0xa4, 0xa6, 0x53, 0xe8, 0x13, 0xfc};
//*/    
    srand(42);
    
    uint8_t key_a[16]; uint8_t key_b[16];
    uint8_t  pt_a[16]; uint8_t  pt_b[16];
    
    for (int i = 0; i < 16; i++) {
        key_a[i] = rand() & 0xff;
        key_b[i] = key[i] ^ key_a[i];
        
        pt_a[i] = rand() & 0xff;
        pt_b[i] = pt[i] ^ pt_a[i];
    }

    //top.p_ClkxCI.set<bool, true>(false);
    top.p_StartxSI.set<bool, true>(false);
    top.p_RstxBI.set<bool, true>(false);
    top.p_KxDI.set<uint16_t, true>(0);
    top.p_PTxDI.set<uint16_t, true>(0);
    prepare_step(manager, top);

    // Computation starts this cycle
    manager.begin_measure();
    top.p_StartxSI.set<bool, true>(true);
    top.p_RstxBI.set<bool, true>(true);
    top.p_KxDI.set<uint16_t, true>(0);
    top.p_PTxDI.set<uint16_t, true>(0);
    prepare_step(manager, top);

    top.p_StartxSI.set<bool, true>(false);

    // Fill it now (One per cycle)
    for (int i = 0; i < 16; i++) {
        #ifdef CONST_EXEC
        Node& concSskey = Concat(constant(key_a[i], 8), constant(key_b[i], 8));
        Node& concSspt = Concat(constant(pt_a[i], 8), constant(pt_b[i], 8));
        #else
        Node& skey = symbol(("key" + std::to_string(i)).c_str(), 'S', 8);
        Node& spt = symbol(("pt" + std::to_string(i)).c_str(), 'S', 8);
        std::vector<Node*> sskey = manager.get_shares(skey, 2);
        std::vector<Node*> sspt = manager.get_shares(spt, 2);
        Node& concSskey = Concat(*sskey[0], *sskey[1]);
        Node& concSspt = Concat(*sspt[0], *sspt[1]);
        #endif

        top.p_KxDI.set<uint16_t>((key_a[i] << 8) + key_b[i]);
        top.p_PTxDI.set<uint16_t>((pt_a[i] << 8) + pt_b[i]);

        top.p_KxDI.setNode(&concSskey);
        top.p_PTxDI.setNode(&concSspt);
        top.p_KxDI.set_fully_unstable();
        top.p_PTxDI.set_fully_unstable();

        prepare_step(manager, top);
    }
    top.p_KxDI.set<uint16_t, true>(0);
    top.p_PTxDI.set<uint16_t, true>(0);
    top.p_KxDI.set_fully_stable();
    top.p_PTxDI.set_fully_stable();

    std::cout << "Starting compute at step " << manager.get_steps() << std::endl; 
    while (top.p_DonexSO.get<bool>() != true && manager.get_steps() < 6000) {
        prepare_step(manager, top);
    }
    
    std::cout << "Stopping ticks, DonexSO is : " << top.p_DonexSO << ", at step " << manager.get_steps() << std::endl;

    uint8_t ct_a[16]; uint8_t ct_b[16];
    uint8_t rct[16];
    Node* res_a[16] = {nullptr}; Node* res_b[16] = {nullptr};
    Node* res_c[16] = {nullptr};
    
    for (int i = 0; i < 16; i++) {
        uint16_t c_ab = top.p_CxDO.get<uint16_t>();
        ct_a[i] = (c_ab >> 8) & 0xff;
        ct_b[i] = c_ab & 0xff;
        rct[i] = ct_a[i] ^ ct_b[i];

        Node* c_abNode = top.p_CxDO.getNode();
        if (c_abNode == nullptr) {
            std::cout << "Tried getting node CxDO_" << i << " but it was null" << std::endl;
        } else {
            std::cout << "Getting node CxDO_" << i << " : " << c_abNode << std::endl;
            res_a[i] = &Extract(15, 8, *c_abNode);
            res_b[i] = &Extract(7, 0, *c_abNode);
            res_c[i] = &(*(res_a[i]) ^ *(res_b[i]));
        }

        prepare_step(manager, top);
    }

    manager.end_measure();
    manager.stat();

    printf("key:  ");
    for(int i = 0; i < 16; i++) printf("%02x", key[i]);
    printf("\n");
      
    printf(" pt:  ");
    for(int i = 0; i < 16; i++) printf("%02x", pt[i]);
    printf("\n");
      
    printf("rct:  ");
    for(int i = 0; i < 16; i++) printf("%02x", rct[i]);
    printf("\n");
      
    printf("ect:  ");
    for(int i = 0; i < 16; i++) printf("%02x", ect[i]);
    printf("\n");
      
    printf("resc: ");
    for(int i = 0; i < 16; i++) if (res_c[i] != nullptr) printf("%02x", (unsigned int)res_c[i]->cst[0]);
    printf("\n");

    std::cout << std::endl << "Unrolled " << manager.get_steps() << " steps." << std::endl;
}

