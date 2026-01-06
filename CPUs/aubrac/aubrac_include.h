#ifndef AUBRAC_INCLUDE_H
#define AUBRAC_INCLUDE_H

#if defined CFG_secure
#define CORE_VERSION "aubrac_secure"
#include "cxxrtl_aubrac_secure.h"
#elif defined CFG_secfast
#define CORE_VERSION "aubrac_secfast"
#include "cxxrtl_aubrac_secfast.h"
#else
#define CORE_VERSION "aubrac_unsecure"
#include "cxxrtl_aubrac_unsecure.h"
#endif

#endif // AUBRAC_INCLUDE_H
