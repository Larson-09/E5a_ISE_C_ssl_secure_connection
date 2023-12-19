//
// Created by jordan on 19/12/23.
//

#ifndef E5A_ISE_C_SSL_SECURE_CONNECTION_TRACE_H
#define E5A_ISE_C_SSL_SECURE_CONNECTION_TRACE_H

#define DEBUG 1

#if DEBUG
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

#endif //E5A_ISE_C_SSL_SECURE_CONNECTION_TRACE_H
