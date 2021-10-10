/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 *
 *  (C) 2001 by Argonne National Laboratory.
 *  (C) 2015 by Microsoft Corporation.
 *
 *                                 MPICH COPYRIGHT
 *
 *  The following is a notice of limited availability of the code, and disclaimer
 *  which must be included in the prologue of the code and in all source listings
 *  of the code.
 *
 *  Copyright Notice
 *   + 2002 University of Chicago
 *
 *  Permission is hereby granted to use, reproduce, prepare derivative works, and
 *  to redistribute to others.  This software was authored by:
 *
 *  Mathematics and Computer Science Division
 *  Argonne National Laboratory, Argonne IL 60439
 *
 *  (and)
 *
 *  Department of Computer Science
 *  University of Illinois at Urbana-Champaign
 *
 *
 *                                GOVERNMENT LICENSE
 *
 *  Portions of this material resulted from work developed under a U.S.
 *  Government Contract and are subject to the following license: the Government
 *  is granted for itself and others acting on its behalf a paid-up, nonexclusive,
 *  irrevocable worldwide license in this computer software to reproduce, prepare
 *  derivative works, and perform publicly and display publicly.
 *
 *                                    DISCLAIMER
 *
 *  This computer code material was prepared, in part, as an account of work
 *  sponsored by an agency of the United States Government.  Neither the United
 *  States, nor the University of Chicago, nor any of their employees, makes any
 *  warranty express or implied, or assumes any legal liability or responsibility
 *  for the accuracy, completeness, or usefulness of any information, apparatus,
 *  product, or process disclosed, or represents that its use would not infringe
 *  privately owned rights.
 *
 */

#ifndef MPI_INCLUDED
#define MPI_INCLUDED

#include <stdint.h>
#ifndef MSMPI_NO_SAL
#include <sal.h>
#endif

#if defined(_MSC_VER) && _MSC_VER < 1600 
typedef signed __int64 _MSMPI_int64_t;
#else 
#include <stdint.h> 
typedef int64_t _MSMPI_int64_t;
#endif

#if defined(__cplusplus)
extern "C" {
#endif


#ifndef MSMPI_VER
#define MSMPI_VER 0x100
#endif


/*---------------------------------------------------------------------------*/
/* SAL ANNOTATIONS                                                           */
/*---------------------------------------------------------------------------*/
/*
 * Define SAL annotations if they aren't defined yet.
 */
#ifndef _Success_
#define _Success_( x )
#endif
#ifndef _Notref_
#define _Notref_
#endif
#ifndef _When_
#define _When_( x, y )
#endif
#ifndef _Pre_valid_
#define _Pre_valid_
#endif
#ifndef _Pre_opt_valid_
#define _Pre_opt_valid_
#endif
#ifndef _Post_invalid_
#define _Post_invalid_
#endif
#ifndef _In_
#define _In_
#endif
#ifndef _In_z_
#define _In_z_
#endif
#ifndef _In_opt_
#define _In_opt_
#endif
#ifndef _In_range_
#define _In_range_( x, y )
#endif
#ifndef _In_reads_
#define _In_reads_( x )
#endif
#ifndef _In_reads_z_
#define _In_reads_z_( x )
#endif
#ifndef _In_reads_opt_
#define _In_reads_opt_( x )
#endif
#ifndef _In_reads_bytes_opt_
#define _In_reads_bytes_opt_( x )
#endif
#ifndef _Out_
#define _Out_
#endif
#ifndef _Out_opt_
#define _Out_opt_
#endif
#ifndef _Out_writes_
#define _Out_writes_( x )
#endif
#ifndef _Out_writes_z_
#define _Out_writes_z_( x )
#endif
#ifndef _Out_writes_opt_
#define _Out_writes_opt_( x )
#endif
#ifndef _Out_writes_to_opt_
#define _Out_writes_to_opt_( x, y )
#endif
#ifndef _Out_writes_bytes_opt_
#define _Out_writes_bytes_opt_( x )
#endif
#ifndef _Inout_
#define _Inout_
#endif
#ifndef _Inout_opt_
#define _Inout_opt_
#endif
#ifndef _Inout_updates_opt_
#define _Inout_updates_opt_( x )
#endif
#ifndef _Deref_in_range_
#define _Deref_in_range_( x, y )
#endif
#ifndef _Deref_out_range_
#define _Deref_out_range_( x, y )
#endif
#ifndef _Pre_satisfies_
#define _Pre_satisfies_( x )
#endif
#ifndef _Post_satisfies_
#define _Post_satisfies_( x )
#endif
#ifndef _Post_equal_to_
#define _Post_equal_to_( x )
#endif

#define _mpi_updates_(size) _When_(size != 0, _Inout_updates_(size))
#define _mpi_reads_(size) _When_(size != 0, _In_reads_(size))
#define _mpi_reads_bytes_(size) _When_(size != 0, _In_reads_bytes_(size))
#define _mpi_writes_(size) _When_(size != 0, _Out_writes_(size))
#define _mpi_writes_bytes_(size) _When_(size != 0, _Out_writes_bytes_(size))
#define _mpi_writes_to_(size, count) _When_(size != 0, _Out_writes_to_(size, count))
#define _mpi_out_flag_ _Out_ _Deref_out_range_(0, 1)
#define _mpi_out_(param, sentinel) _Out_ _Post_satisfies_(*param == sentinel || *param >= 0)
#define _mpi_out_range_(param, sentinel, ub) \
    _Out_ _Post_satisfies_(*param == sentinel || (ub > 0 && *param >= 0 && *param <= ub))
#define _mpi_position_(ub) _Inout_ _Deref_in_range_(0, ub) _Deref_out_range_(0, ub)
#define _mpi_coll_rank_(param) _In_ _Pre_satisfies_(param == MPI_ROOT || param >= MPI_PROC_NULL)

/*---------------------------------------------------------------------------*/
/* MSMPI Calling convention                                                  */
/*---------------------------------------------------------------------------*/

#define MPIAPI __stdcall


/*---------------------------------------------------------------------------*/
/* MPI ERROR CLASS                                                           */
/*---------------------------------------------------------------------------*/

#define MPI_SUCCESS          0      /* Successful return code */

#define MPI_ERR_BUFFER       1      /* Invalid buffer pointer */
#define MPI_ERR_COUNT        2      /* Invalid count argument */
#define MPI_ERR_TYPE         3      /* Invalid datatype argument */
#define MPI_ERR_TAG          4      /* Invalid tag argument */
#define MPI_ERR_COMM         5      /* Invalid communicator */
#define MPI_ERR_RANK         6      /* Invalid rank */
#define MPI_ERR_ROOT         7      /* Invalid root */
#define MPI_ERR_GROUP        8      /* Invalid group */
#define MPI_ERR_OP           9      /* Invalid operation */
#define MPI_ERR_TOPOLOGY    10      /* Invalid topology */
#define MPI_ERR_DIMS        11      /* Invalid dimension argument */
#define MPI_ERR_ARG         12      /* Invalid argument */
#define MPI_ERR_UNKNOWN     13      /* Unknown error */
#define MPI_ERR_TRUNCATE    14      /* Message truncated on receive */
#define MPI_ERR_OTHER       15      /* Other error; use Error_string */
#define MPI_ERR_INTERN      16      /* Internal error code */
#define MPI_ERR_IN_STATUS   17      /* Error code is in status */
#define MPI_ERR_PENDING     18      /* Pending request */
#define MPI_ERR_REQUEST     19      /* Invalid request (handle) */
#define MPI_ERR_ACCESS      20      /* Premission denied */
#define MPI_ERR_AMODE       21      /* Error related to amode passed to MPI_File_open */
#define MPI_ERR_BAD_FILE    22      /* Invalid file name (e.g., path name too long) */
#define MPI_ERR_CONVERSION  23      /* Error in user data conversion function */
#define MPI_ERR_DUP_DATAREP 24      /* Data representation identifier already registered */
#define MPI_ERR_FILE_EXISTS 25      /* File exists */
#define MPI_ERR_FILE_IN_USE 26      /* File operation could not be completed, file in use */
#define MPI_ERR_FILE        27      /* Invalid file handle */
#define MPI_ERR_INFO        28      /* Invalid info argument */
#define MPI_ERR_INFO_KEY    29      /* Key longer than MPI_MAX_INFO_KEY */
#define MPI_ERR_INFO_VALUE  30      /* Value longer than MPI_MAX_INFO_VAL */
#define MPI_ERR_INFO_NOKEY  31      /* Invalid key passed to MPI_Info_delete */
#define MPI_ERR_IO          32      /* Other I/O error */
#define MPI_ERR_NAME        33      /* Invalid service name in MPI_Lookup_name */
#define MPI_ERR_NO_MEM      34      /* Alloc_mem could not allocate memory */
#define MPI_ERR_NOT_SAME    35      /* Collective argument/sequence not the same on all processes */
#define MPI_ERR_NO_SPACE    36      /* Not enough space */
#define MPI_ERR_NO_SUCH_FILE 37     /* File does not exist */
#define MPI_ERR_PORT        38      /* Invalid port name in MPI_comm_connect*/
#define MPI_ERR_QUOTA       39      /* Quota exceeded */
#define MPI_ERR_READ_ONLY   40      /* Read-only file or file system */
#define MPI_ERR_SERVICE     41      /* Invalid service name in MPI_Unpublish_name */
#define MPI_ERR_SPAWN       42      /* Error in spawning processes */
#define MPI_ERR_UNSUPPORTED_DATAREP   43  /* Unsupported dararep in MPI_File_set_view */
#define MPI_ERR_UNSUPPORTED_OPERATION 44  /* Unsupported operation on file */
#define MPI_ERR_WIN         45      /* Invalid win argument */
#define MPI_ERR_BASE        46      /* Invalid base passed to MPI_Free_mem */
#define MPI_ERR_LOCKTYPE    47      /* Invalid locktype argument */
#define MPI_ERR_KEYVAL      48      /* Invalid keyval  */
#define MPI_ERR_RMA_CONFLICT 49     /* Conflicting accesses to window */
#define MPI_ERR_RMA_SYNC    50      /* Wrong synchronization of RMA calls */
#define MPI_ERR_SIZE        51      /* Invalid size argument */
#define MPI_ERR_DISP        52      /* Invalid disp argument */
#define MPI_ERR_ASSERT      53      /* Invalid assert argument */

#define MPI_ERR_LASTCODE    0x3fffffff    /* Last valid error code for a predefined error class */

#define MPICH_ERR_LAST_CLASS 53


/*---------------------------------------------------------------------------*/
/* MPI Basic integer types                                                   */
/*---------------------------------------------------------------------------*/

/* Address size integer */
#ifdef _WIN64
typedef _MSMPI_int64_t MPI_Aint;
#else
typedef int MPI_Aint;
#endif

/* Fortran INTEGER */
typedef int MPI_Fint;

/* File offset */
typedef _MSMPI_int64_t MPI_Offset;

//
// MPI-3 standard defines this type that can be used to address locations
// within either memory or files as well as express count values.
//
typedef _MSMPI_int64_t MPI_Count;


/*---------------------------------------------------------------------------*/
/* MPI_Datatype                                                              */
/*---------------------------------------------------------------------------*/

typedef int MPI_Datatype;
#define MPI_DATATYPE_NULL           ((MPI_Datatype)0x0c000000)

#define MPI_CHAR                    ((MPI_Datatype)0x4c000101)
#define MPI_UNSIGNED_CHAR           ((MPI_Datatype)0x4c000102)
#define MPI_SHORT                   ((MPI_Datatype)0x4c000203)
#define MPI_UNSIGNED_SHORT          ((MPI_Datatype)0x4c000204)
#define MPI_INT                     ((MPI_Datatype)0x4c000405)
#define MPI_UNSIGNED                ((MPI_Datatype)0x4c000406)
#define MPI_LONG                    ((MPI_Datatype)0x4c000407)
#define MPI_UNSIGNED_LONG           ((MPI_Datatype)0x4c000408)
#define MPI_LONG_LONG_INT           ((MPI_Datatype)0x4c000809)
#define MPI_LONG_LONG               MPI_LONG_LONG_INT
#define MPI_FLOAT                   ((MPI_Datatype)0x4c00040a)
#define MPI_DOUBLE                  ((MPI_Datatype)0x4c00080b)
#define MPI_LONG_DOUBLE             ((MPI_Datatype)0x4c00080c)
#define MPI_BYTE                    ((MPI_Datatype)0x4c00010d)
#define MPI_WCHAR                   ((MPI_Datatype)0x4c00020e)

#define MPI_PACKED                  ((MPI_Datatype)0x4c00010f)
#define MPI_LB                      ((MPI_Datatype)0x4c000010)
#define MPI_UB                      ((MPI_Datatype)0x4c000011)

#define MPI_C_COMPLEX               ((MPI_Datatype)0x4c000812)
#define MPI_C_FLOAT_COMPLEX         ((MPI_Datatype)0x4c000813)
#define MPI_C_DOUBLE_COMPLEX        ((MPI_Datatype)0x4c001014)
#define MPI_C_LONG_DOUBLE_COMPLEX   ((MPI_Datatype)0x4c001015)

#define MPI_2INT                    ((MPI_Datatype)0x4c000816)
#define MPI_C_BOOL                  ((MPI_Datatype)0x4c000117)
#define MPI_SIGNED_CHAR             ((MPI_Datatype)0x4c000118)
#define MPI_UNSIGNED_LONG_LONG      ((MPI_Datatype)0x4c000819)

/* Fortran types */
#define MPI_CHARACTER               ((MPI_Datatype)0x4c00011a)
#define MPI_INTEGER                 ((MPI_Datatype)0x4c00041b)
#define MPI_REAL                    ((MPI_Datatype)0x4c00041c)
#define MPI_LOGICAL                 ((MPI_Datatype)0x4c00041d)
#define MPI_COMPLEX                 ((MPI_Datatype)0x4c00081e)
#define MPI_DOUBLE_PRECISION        ((MPI_Datatype)0x4c00081f)
#define MPI_2INTEGER                ((MPI_Datatype)0x4c000820)
#define MPI_2REAL                   ((MPI_Datatype)0x4c000821)
#define MPI_DOUBLE_COMPLEX          ((MPI_Datatype)0x4c001022)
#define MPI_2DOUBLE_PRECISION       ((MPI_Datatype)0x4c001023)
#define MPI_2COMPLEX                ((MPI_Datatype)0x4c001024)
#define MPI_2DOUBLE_COMPLEX         ((MPI_Datatype)0x4c002025)

/* Size-specific types (see MPI 2.2, 16.2.5) */
#define MPI_REAL2                   MPI_DATATYPE_NULL
#define MPI_REAL4                   ((MPI_Datatype)0x4c000427)
#define MPI_COMPLEX8                ((MPI_Datatype)0x4c000828)
#define MPI_REAL8                   ((MPI_Datatype)0x4c000829)
#define MPI_COMPLEX16               ((MPI_Datatype)0x4c00102a)
#define MPI_REAL16                  MPI_DATATYPE_NULL
#define MPI_COMPLEX32               MPI_DATATYPE_NULL
#define MPI_INTEGER1                ((MPI_Datatype)0x4c00012d)
#define MPI_COMPLEX4                MPI_DATATYPE_NULL
#define MPI_INTEGER2                ((MPI_Datatype)0x4c00022f)
#define MPI_INTEGER4                ((MPI_Datatype)0x4c000430)
#define MPI_INTEGER8                ((MPI_Datatype)0x4c000831)
#define MPI_INTEGER16               MPI_DATATYPE_NULL
#define MPI_INT8_T                  ((MPI_Datatype)0x4c000133)
#define MPI_INT16_T                 ((MPI_Datatype)0x4c000234)
#define MPI_INT32_T                 ((MPI_Datatype)0x4c000435)
#define MPI_INT64_T                 ((MPI_Datatype)0x4c000836)
#define MPI_UINT8_T                 ((MPI_Datatype)0x4c000137)
#define MPI_UINT16_T                ((MPI_Datatype)0x4c000238)
#define MPI_UINT32_T                ((MPI_Datatype)0x4c000439)
#define MPI_UINT64_T                ((MPI_Datatype)0x4c00083a)

#ifdef _WIN64
#define MPI_AINT                    ((MPI_Datatype)0x4c00083b)
#else
#define MPI_AINT                    ((MPI_Datatype)0x4c00043b)
#endif
#define MPI_OFFSET                  ((MPI_Datatype)0x4c00083c)
#define MPI_COUNT                   ((MPI_Datatype)0x4c00083d)

/*
 * The layouts for the types MPI_DOUBLE_INT etc. are
 *
 *      struct { double a; int b; }
 */
#define MPI_FLOAT_INT               ((MPI_Datatype)0x8c000000)
#define MPI_DOUBLE_INT              ((MPI_Datatype)0x8c000001)
#define MPI_LONG_INT                ((MPI_Datatype)0x8c000002)
#define MPI_SHORT_INT               ((MPI_Datatype)0x8c000003)
#define MPI_LONG_DOUBLE_INT         ((MPI_Datatype)0x8c000004)


/*---------------------------------------------------------------------------*/
/* MPI_Comm                                                                  */
/*---------------------------------------------------------------------------*/

typedef int MPI_Comm;
#define MPI_COMM_NULL  ((MPI_Comm)0x04000000)

#define MPI_COMM_WORLD ((MPI_Comm)0x44000000)
#define MPI_COMM_SELF  ((MPI_Comm)0x44000001)

/*---------------------------------------------------------------------------*/
/* MPI_Comm Split Types                                                      */
/*---------------------------------------------------------------------------*/
enum
{
    MPI_COMM_TYPE_SHARED    = 1,
};


/*---------------------------------------------------------------------------*/
/* MPI_Win                                                                   */
/*---------------------------------------------------------------------------*/

typedef int MPI_Win;
#define MPI_WIN_NULL ((MPI_Win)0x20000000)


/*---------------------------------------------------------------------------*/
/* MPI_File                                                                  */
/*---------------------------------------------------------------------------*/

typedef struct ADIOI_FileD* MPI_File;
#define MPI_FILE_NULL ((MPI_File)0)


/*---------------------------------------------------------------------------*/
/* MPI_Op                                                                    */
/*---------------------------------------------------------------------------*/

typedef int MPI_Op;
#define MPI_OP_NULL ((MPI_Op)0x18000000)

#define MPI_MAX     ((MPI_Op)0x58000001)
#define MPI_MIN     ((MPI_Op)0x58000002)
#define MPI_SUM     ((MPI_Op)0x58000003)
#define MPI_PROD    ((MPI_Op)0x58000004)
#define MPI_LAND    ((MPI_Op)0x58000005)
#define MPI_BAND    ((MPI_Op)0x58000006)
#define MPI_LOR     ((MPI_Op)0x58000007)
#define MPI_BOR     ((MPI_Op)0x58000008)
#define MPI_LXOR    ((MPI_Op)0x58000009)
#define MPI_BXOR    ((MPI_Op)0x5800000a)
#define MPI_MINLOC  ((MPI_Op)0x5800000b)
#define MPI_MAXLOC  ((MPI_Op)0x5800000c)
#define MPI_REPLACE ((MPI_Op)0x5800000d)
#define MPI_NO_OP   ((MPI_Op)0x5800000e)


/*---------------------------------------------------------------------------*/
/* MPI_Info                                                                  */
/*---------------------------------------------------------------------------*/

typedef int MPI_Info;
#define MPI_INFO_NULL         ((MPI_Info)0x1c000000)


/*---------------------------------------------------------------------------*/
/* MPI_Request                                                               */
/*---------------------------------------------------------------------------*/

typedef int MPI_Request;
#define MPI_REQUEST_NULL ((MPI_Request)0x2c000000)


/*---------------------------------------------------------------------------*/
/* MPI_Group                                                                 */
/*---------------------------------------------------------------------------*/

typedef int MPI_Group;
#define MPI_GROUP_NULL  ((MPI_Group)0x08000000)

#define MPI_GROUP_EMPTY ((MPI_Group)0x48000000)


/*---------------------------------------------------------------------------*/
/* MPI_Errhandler                                                            */
/*---------------------------------------------------------------------------*/

typedef int MPI_Errhandler;
#define MPI_ERRHANDLER_NULL  ((MPI_Errhandler)0x14000000)

#define MPI_ERRORS_ARE_FATAL ((MPI_Errhandler)0x54000000)
#define MPI_ERRORS_RETURN    ((MPI_Errhandler)0x54000001)


/*---------------------------------------------------------------------------*/
/* MPI_Message                                                               */
/*---------------------------------------------------------------------------*/

typedef int MPI_Message;
#define MPI_MESSAGE_NULL     ((MPI_Message)0x30000000)
#define MPI_MESSAGE_NO_PROC  ((MPI_Message)0x70000000)

/*---------------------------------------------------------------------------*/
/* MPI_Status                                                                */
/*---------------------------------------------------------------------------*/

typedef struct MPI_Status
{
    int internal[2];

    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;

} MPI_Status;

#define MPI_STATUS_IGNORE ((MPI_Status*)(MPI_Aint)1)
#define MPI_STATUSES_IGNORE ((MPI_Status*)(MPI_Aint)1)


/*---------------------------------------------------------------------------*/
/* MISC CONSTANTS                                                            */
/*---------------------------------------------------------------------------*/

/* Used in: Count, Index, Rank, Color, Toplogy, Precision, Exponent range  */
#define MPI_UNDEFINED   (-32766)

/* Used in: Rank */
#define MPI_PROC_NULL   (-1)
#define MPI_ANY_SOURCE  (-2)
#define MPI_ROOT        (-3)

/* Used in: Tag */
#define MPI_ANY_TAG     (-1)

/* Used for: Buffer address */
#define MPI_BOTTOM        ((void*)0)
#define MPI_UNWEIGHTED    ((int*)1)
#define MPI_WEIGHTS_EMPTY ((int*)2)

/*---------------------------------------------------------------------------*/
/* Macro for function return values.                                         */
/*---------------------------------------------------------------------------*/
#define MPI_METHOD _Success_( return == MPI_SUCCESS ) int MPIAPI


/*---------------------------------------------------------------------------*/
/* Chapter 3: Point-to-Point Communication                                   */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------*/
/* Section 3.2: Blocking Communication         */
/*---------------------------------------------*/

MPI_METHOD
MPI_Send(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm
    );

MPI_METHOD
PMPI_Send(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm
    );

MPI_METHOD
MPI_Recv(
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_Recv(
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Status* status
    );

_Pre_satisfies_(status != MPI_STATUS_IGNORE)
MPI_METHOD
MPI_Get_count(
    _In_ const MPI_Status* status,
    _In_ MPI_Datatype datatype,
    _mpi_out_(count, MPI_UNDEFINED) int* count
    );

_Pre_satisfies_(status != MPI_STATUS_IGNORE)
MPI_METHOD
PMPI_Get_count(
    _In_ const MPI_Status* status,
    _In_ MPI_Datatype datatype,
    _mpi_out_(count, MPI_UNDEFINED) int* count
    );


/*---------------------------------------------*/
/* Section 3.4: Communication Modes            */
/*---------------------------------------------*/

MPI_METHOD
MPI_Bsend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm
    );

MPI_METHOD
PMPI_Bsend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm
    );

MPI_METHOD
MPI_Ssend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm
    );

MPI_METHOD
PMPI_Ssend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm
    );

MPI_METHOD
MPI_Rsend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm
    );

MPI_METHOD
PMPI_Rsend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm
    );


/*---------------------------------------------*/
/* Section 3.6: Buffer Allocation              */
/*---------------------------------------------*/

/* Upper bound on bsend overhead for each message */
#define MSMPI_BSEND_OVERHEAD_V1   95
#define MSMPI_BSEND_OVERHEAD_V2   MSMPI_BSEND_OVERHEAD_V1

#if MSMPI_VER > 0x300
#  define MPI_BSEND_OVERHEAD  MSMPI_Get_bsend_overhead()
#else
#  define MPI_BSEND_OVERHEAD  MSMPI_BSEND_OVERHEAD_V1
#endif

MPI_METHOD
MPI_Buffer_attach(
    _In_ void* buffer,
    _In_range_(>=, 0) int size
    );

MPI_METHOD
PMPI_Buffer_attach(
    _In_ void* buffer,
    _In_range_(>=, 0) int size
    );

MPI_METHOD
MPI_Buffer_detach(
    _Out_ void* buffer_addr,
    _Out_ int* size
    );

MPI_METHOD
PMPI_Buffer_detach(
    _Out_ void* buffer_addr,
    _Out_ int* size
    );


/*---------------------------------------------*/
/* Section 3.7: Nonblocking Communication      */
/*---------------------------------------------*/

MPI_METHOD
MPI_Isend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Isend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_Ibsend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Ibsend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_Issend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Issend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_Irsend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Irsend(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_Irecv(
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Irecv(
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );


/*---------------------------------------------*/
/* Section 3.7.3: Communication Completion     */
/*---------------------------------------------*/

MPI_METHOD
MPI_Wait(
    _Inout_ _Post_equal_to_(MPI_REQUEST_NULL) MPI_Request* request,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_Wait(
    _Inout_ _Post_equal_to_(MPI_REQUEST_NULL) MPI_Request* request,
    _Out_ MPI_Status* status
    );

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
MPI_Test(
    _Inout_ _Post_equal_to_(MPI_REQUEST_NULL) MPI_Request* request,
    _mpi_out_flag_ int* flag,
    _Out_ MPI_Status* status
    );

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
PMPI_Test(
    _Inout_ _Post_equal_to_(MPI_REQUEST_NULL) MPI_Request* request,
    _mpi_out_flag_ int* flag,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_Request_free(
    _Inout_ _Post_equal_to_(MPI_REQUEST_NULL) MPI_Request* request
    );

MPI_METHOD
PMPI_Request_free(
    _Inout_ _Post_equal_to_(MPI_REQUEST_NULL) MPI_Request* request
    );


/*---------------------------------------------*/
/* Section 3.7.5: Multiple Completions         */
/*---------------------------------------------*/

MPI_METHOD
MPI_Waitany(
    _In_range_(>=, 0) int count,
    _mpi_updates_(count) MPI_Request array_of_requests[],
    _mpi_out_range_(index, MPI_UNDEFINED, (count - 1)) int* index,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_Waitany(
    _In_range_(>=, 0) int count,
    _mpi_updates_(count) MPI_Request array_of_requests[],
    _mpi_out_range_(index, MPI_UNDEFINED, (count - 1)) int* index,
    _Out_ MPI_Status* status
    );

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
MPI_Testany(
    _In_range_(>=, 0) int count,
    _mpi_updates_(count) MPI_Request array_of_requests[],
    _mpi_out_range_(index, MPI_UNDEFINED, (count - 1)) int* index,
    _mpi_out_flag_ int* flag,
    _Out_ MPI_Status* status
    );

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
PMPI_Testany(
    _In_range_(>=, 0) int count,
    _mpi_updates_(count) MPI_Request array_of_requests[],
    _mpi_out_range_(index, MPI_UNDEFINED, (count - 1)) int* index,
    _mpi_out_flag_ int* flag,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_Waitall(
    _In_range_(>=, 0) int count,
    _mpi_updates_(count) MPI_Request array_of_requests[],
    _Out_writes_opt_(count) MPI_Status array_of_statuses[]
    );

MPI_METHOD
PMPI_Waitall(
    _In_range_(>=, 0) int count,
    _mpi_updates_(count) MPI_Request array_of_requests[],
    _Out_writes_opt_(count) MPI_Status array_of_statuses[]
    );

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
MPI_Testall(
    _In_range_(>=, 0) int count,
    _mpi_updates_(count) MPI_Request array_of_requests[],
    _mpi_out_flag_ int* flag,
    _Out_writes_opt_(count) MPI_Status array_of_statuses[]
    );

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
PMPI_Testall(
    _In_range_(>=, 0) int count,
    _mpi_updates_(count) MPI_Request array_of_requests[],
    _mpi_out_flag_ int* flag,
    _Out_writes_opt_(count) MPI_Status array_of_statuses[]
);

MPI_METHOD
MPI_Waitsome(
    _In_range_(>=, 0) int incount,
    _mpi_updates_(incount) MPI_Request array_of_requests[],
    _mpi_out_range_(outcount, MPI_UNDEFINED, incount) int* outcount,
    _mpi_writes_to_(incount,*outcount) int array_of_indices[],
    _Out_writes_to_opt_(incount, *outcount) MPI_Status array_of_statuses[]
    );

MPI_METHOD
PMPI_Waitsome(
    _In_range_(>=, 0) int incount,
    _mpi_updates_(incount) MPI_Request array_of_requests[],
    _mpi_out_range_(outcount, MPI_UNDEFINED, incount) int* outcount,
    _mpi_writes_to_(incount,*outcount) int array_of_indices[],
    _Out_writes_to_opt_(incount, *outcount) MPI_Status array_of_statuses[]
    );

_Success_(return == MPI_SUCCESS && *outcount > 0)
int
MPIAPI
MPI_Testsome(
    _In_range_(>=, 0) int incount,
    _mpi_updates_(incount) MPI_Request array_of_requests[],
    _mpi_out_range_(outcount, MPI_UNDEFINED, incount) int* outcount,
    _mpi_writes_to_(incount,*outcount) int array_of_indices[],
    _Out_writes_to_opt_(incount, *outcount) MPI_Status array_of_statuses[]
    );

_Success_(return == MPI_SUCCESS && *outcount > 0)
int
MPIAPI
PMPI_Testsome(
    _In_range_(>=, 0) int incount,
    _mpi_updates_(incount) MPI_Request array_of_requests[],
    _mpi_out_range_(outcount, MPI_UNDEFINED, incount) int* outcount,
    _mpi_writes_to_(incount,*outcount) int array_of_indices[],
    _Out_writes_to_opt_(incount, *outcount) MPI_Status array_of_statuses[]
    );


/*---------------------------------------------*/
/* Section 3.7.6: Test of status               */
/*---------------------------------------------*/

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
MPI_Request_get_status(
    _In_ MPI_Request request,
    _mpi_out_flag_ int* flag,
    _Out_ MPI_Status* status
    );

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
PMPI_Request_get_status(
    _In_ MPI_Request request,
    _mpi_out_flag_ int* flag,
    _Out_ MPI_Status* status
    );


/*---------------------------------------------*/
/* Section 3.8: Probe and Cancel               */
/*---------------------------------------------*/

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
MPI_Iprobe(
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _mpi_out_flag_ int* flag,
    _Out_ MPI_Status* status
    );

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
PMPI_Iprobe(
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _mpi_out_flag_ int* flag,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_Probe(
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_Probe(
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Status* status
    );

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
MPI_Improbe(
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _mpi_out_flag_ int* flag,
    _Out_ MPI_Message* message,
    _Out_ MPI_Status* status
    );

_Success_(return == MPI_SUCCESS && *flag != 0)
int
MPIAPI
PMPI_Improbe(
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _mpi_out_flag_ int* flag,
    _Out_ MPI_Message* message,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_Mprobe(
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Message* message,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_Mprobe(
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Message* message,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_Mrecv(
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Inout_ MPI_Message* message,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_Mrecv(
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Inout_ MPI_Message* message,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_Imrecv(
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Inout_ MPI_Message* message,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Imrecv(
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Inout_ MPI_Message* message,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(*request != MPI_REQUEST_NULL)
MPI_METHOD
MPI_Cancel(
    _In_ MPI_Request* request
    );

_Pre_satisfies_(*request != MPI_REQUEST_NULL)
MPI_METHOD
PMPI_Cancel(
    _In_ MPI_Request* request
    );

_Pre_satisfies_(status != MPI_STATUS_IGNORE)
MPI_METHOD
MPI_Test_cancelled(
    _In_ const MPI_Status* status,
    _mpi_out_flag_ int* flag
    );

_Pre_satisfies_(status != MPI_STATUS_IGNORE)
MPI_METHOD
PMPI_Test_cancelled(
    _In_ const MPI_Status* status,
    _mpi_out_flag_ int* flag
    );


/*---------------------------------------------*/
/* Section 3.9: Persistent Communication       */
/*---------------------------------------------*/

MPI_METHOD
MPI_Send_init(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Send_init(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_Bsend_init(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Bsend_init(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_Ssend_init(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Ssend_init(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_Rsend_init(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Rsend_init(
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_Recv_init(
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Recv_init(
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int tag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(*request != MPI_REQUEST_NULL)
MPI_METHOD
MPI_Start(
    _Inout_ MPI_Request* request
    );

_Pre_satisfies_(*request != MPI_REQUEST_NULL)
MPI_METHOD
PMPI_Start(
    _Inout_ MPI_Request* request
    );

MPI_METHOD
MPI_Startall(
    _In_range_(>=, 0) int count,
    _mpi_updates_(count) MPI_Request array_of_requests[]
    );

MPI_METHOD
PMPI_Startall(
    _In_range_(>=, 0) int count,
    _mpi_updates_(count) MPI_Request array_of_requests[]
    );


/*---------------------------------------------*/
/* Section 3.10: Send-Recv                     */
/*---------------------------------------------*/

MPI_METHOD
MPI_Sendrecv(
    _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int sendtag,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int recvtag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_Sendrecv(
    _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int sendtag,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int recvtag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_Sendrecv_replace(
    _Inout_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int sendtag,
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int recvtag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_Sendrecv_replace(
    _Inout_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, MPI_PROC_NULL) int dest,
    _In_range_(>=, 0) int sendtag,
    _In_range_(>=, MPI_ANY_SOURCE) int source,
    _In_range_(>=, MPI_ANY_TAG) int recvtag,
    _In_ MPI_Comm comm,
    _Out_ MPI_Status* status
    );


/*---------------------------------------------------------------------------*/
/* Chapter 4: Datatypes                                                      */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------*/
/* Section 4.1: Derived Datatypes              */
/*---------------------------------------------*/

MPI_METHOD
MPI_Type_contiguous(
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_contiguous(
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
MPI_Type_vector(
    _In_range_(>=, 0) int count,
    _In_range_(>=, 0) int blocklength,
    _In_ int stride,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_vector(
    _In_range_(>=, 0) int count,
    _In_range_(>=, 0) int blocklength,
    _In_ int stride,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
MPI_Type_create_hvector(
    _In_range_(>=, 0) int count,
    _In_range_(>=, 0) int blocklength,
    _In_ MPI_Aint stride,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_create_hvector(
    _In_range_(>=, 0) int count,
    _In_range_(>=, 0) int blocklength,
    _In_ MPI_Aint stride,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
MPI_Type_indexed(
    _In_range_(>=, 0) int count,
    _mpi_reads_(count) const int array_of_blocklengths[],
    _mpi_reads_(count) const int array_of_displacements[],
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_indexed(
    _In_range_(>=, 0) int count,
    _mpi_reads_(count) const int array_of_blocklengths[],
    _mpi_reads_(count) const int array_of_displacements[],
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
MPI_Type_create_hindexed(
    _In_range_(>=, 0) int count,
    _mpi_reads_(count) const int array_of_blocklengths[],
    _mpi_reads_(count) const MPI_Aint array_of_displacements[],
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_create_hindexed(
    _In_range_(>=, 0) int count,
    _mpi_reads_(count) const int array_of_blocklengths[],
    _mpi_reads_(count) const MPI_Aint array_of_displacements[],
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
MPI_Type_create_hindexed_block(
    _In_range_(>=, 0) int count,
    _In_range_(>=, 0) int blocklength,
    _mpi_reads_(count) const MPI_Aint array_of_displacements[],
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_create_hindexed_block(
    _In_range_(>=, 0) int count,
    _In_range_(>=, 0) int blocklength,
    _mpi_reads_(count) const MPI_Aint array_of_displacements[],
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
MPI_Type_create_indexed_block(
    _In_range_(>=, 0) int count,
    _In_range_(>=, 0) int blocklength,
    _mpi_reads_(count) const int array_of_displacements[],
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_create_indexed_block(
    _In_range_(>=, 0) int count,
    _In_range_(>=, 0) int blocklength,
    _mpi_reads_(count) const int array_of_displacements[],
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
MPI_Type_create_struct(
    _In_range_(>=, 0) int count,
    _mpi_reads_(count) const int array_of_blocklengths[],
    _mpi_reads_(count) const MPI_Aint array_of_displacements[],
    _mpi_reads_(count) const MPI_Datatype array_of_types[],
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_create_struct(
    _In_range_(>=, 0) int count,
    _mpi_reads_(count) const int array_of_blocklengths[],
    _mpi_reads_(count) const MPI_Aint array_of_displacements[],
    _mpi_reads_(count) const MPI_Datatype array_of_types[],
    _Out_ MPI_Datatype* newtype
    );


#define MPI_ORDER_C         56
#define MPI_ORDER_FORTRAN   57

MPI_METHOD
MPI_Type_create_subarray(
    _In_range_(>=, 0) int ndims,
    _mpi_reads_(ndims) const int array_of_sizes[],
    _mpi_reads_(ndims) const int array_of_subsizes[],
    _mpi_reads_(ndims) const int array_of_starts[],
    _In_range_(MPI_ORDER_C, MPI_ORDER_FORTRAN) int order,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_create_subarray(
    _In_range_(>=, 0) int ndims,
    _In_reads_opt_(ndims) const int array_of_sizes[],
    _In_reads_opt_(ndims) const int array_of_subsizes[],
    _In_reads_opt_(ndims) const int array_of_starts[],
    _In_range_(MPI_ORDER_C, MPI_ORDER_FORTRAN) int order,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );


#define MPI_DISTRIBUTE_BLOCK         121
#define MPI_DISTRIBUTE_CYCLIC        122
#define MPI_DISTRIBUTE_NONE          123
#define MPI_DISTRIBUTE_DFLT_DARG (-49767)

_Pre_satisfies_(
    order == MPI_DISTRIBUTE_DFLT_DARG ||
    (order >= MPI_DISTRIBUTE_BLOCK && order <= MPI_DISTRIBUTE_NONE)
    )
MPI_METHOD
MPI_Type_create_darray(
    _In_range_(>=, 0) int size,
    _In_range_(>=, 0) int rank,
    _In_range_(>=, 0) int ndims,
    _mpi_reads_(ndims) const int array_of_gsizes[],
    _mpi_reads_(ndims) const int array_of_distribs[],
    _mpi_reads_(ndims) const int array_of_dargs[],
    _mpi_reads_(ndims) const int array_of_psizes[],
    _In_ int order,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

_Pre_satisfies_(
    order == MPI_DISTRIBUTE_DFLT_DARG ||
    (order >= MPI_DISTRIBUTE_BLOCK && order <= MPI_DISTRIBUTE_NONE)
    )
MPI_METHOD
PMPI_Type_create_darray(
    _In_range_(>=, 0) int size,
    _In_range_(>=, 0) int rank,
    _In_range_(>=, 0) int ndims,
    _mpi_reads_(ndims) const int array_of_gsizes[],
    _mpi_reads_(ndims) const int array_of_distribs[],
    _mpi_reads_(ndims) const int array_of_dargs[],
    _mpi_reads_(ndims) const int array_of_psizes[],
    _In_ int order,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );


/*---------------------------------------------*/
/* Section 4.1.5: Datatype Address and Size    */
/*---------------------------------------------*/

MPI_METHOD
MPI_Get_address(
    _In_ const void* location,
    _Out_ MPI_Aint* address
    );

MPI_METHOD
PMPI_Get_address(
    _In_ const void* location,
    _Out_ MPI_Aint* address
    );

MPI_Aint
MPI_Aint_add(
    _In_ MPI_Aint base,
    _In_ MPI_Aint disp
    );

MPI_Aint
PMPI_Aint_add(
    _In_ MPI_Aint base,
    _In_ MPI_Aint disp
    );

MPI_Aint
MPI_Aint_diff(
    _In_ MPI_Aint base,
    _In_ MPI_Aint disp
    );

MPI_Aint
PMPI_Aint_diff(
    _In_ MPI_Aint base,
    _In_ MPI_Aint disp
    );

MPI_METHOD
MPI_Type_size(
    _In_ MPI_Datatype datatype,
    _mpi_out_(size, MPI_UNDEFINED) int* size
    );

MPI_METHOD
PMPI_Type_size(
    _In_ MPI_Datatype datatype,
    _mpi_out_(size, MPI_UNDEFINED) int* size
    );

MPI_METHOD
MPI_Type_size_x(
    _In_ MPI_Datatype datatype,
    _mpi_out_(size, MPI_UNDEFINED) MPI_Count *size
    );

MPI_METHOD
PMPI_Type_size_x(
    _In_ MPI_Datatype datatype,
    _mpi_out_(size, MPI_UNDEFINED) MPI_Count *size
    );


/*---------------------------------------------*/
/* Section 4.1.7: Datatype Extent and Bounds   */
/*---------------------------------------------*/

MPI_METHOD
MPI_Type_get_extent(
    _In_ MPI_Datatype datatype,
    _mpi_out_(lb, MPI_UNDEFINED) MPI_Aint* lb,
    _mpi_out_(extent, MPI_UNDEFINED) MPI_Aint* extent
    );

MPI_METHOD
PMPI_Type_get_extent(
    _In_ MPI_Datatype datatype,
    _mpi_out_(lb, MPI_UNDEFINED) MPI_Aint* lb,
    _mpi_out_(extent, MPI_UNDEFINED) MPI_Aint* extent
    );

MPI_METHOD
MPI_Type_get_extent_x(
    _In_ MPI_Datatype datatype,
    _mpi_out_(lb, MPI_UNDEFINED) MPI_Count *lb,
    _mpi_out_(extent, MPI_UNDEFINED) MPI_Count *extent
    );

MPI_METHOD
PMPI_Type_get_extent_x(
    _In_ MPI_Datatype datatype,
    _mpi_out_(lb, MPI_UNDEFINED) MPI_Count *lb,
    _mpi_out_(extent, MPI_UNDEFINED) MPI_Count *extent
    );

MPI_METHOD
MPI_Type_create_resized(
    _In_ MPI_Datatype oldtype,
    _In_ MPI_Aint lb,
    _In_range_(>=, 0) MPI_Aint extent,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_create_resized(
    _In_ MPI_Datatype oldtype,
    _In_ MPI_Aint lb,
    _In_range_(>=, 0) MPI_Aint extent,
    _Out_ MPI_Datatype* newtype
    );


/*---------------------------------------------*/
/* Section 4.1.8: Datatype True Extent         */
/*---------------------------------------------*/

MPI_METHOD
MPI_Type_get_true_extent(
    _In_ MPI_Datatype datatype,
    _mpi_out_(true_lb, MPI_UNDEFINED) MPI_Aint* true_lb,
    _mpi_out_(true_extent, MPI_UNDEFINED) MPI_Aint* true_extent
    );

MPI_METHOD
PMPI_Type_get_true_extent(
    _In_ MPI_Datatype datatype,
    _mpi_out_(true_lb, MPI_UNDEFINED) MPI_Aint* true_lb,
    _mpi_out_(true_extent, MPI_UNDEFINED) MPI_Aint* true_extent
    );

MPI_METHOD
MPI_Type_get_true_extent_x(
    _In_ MPI_Datatype datatype,
    _mpi_out_(true_lb, MPI_UNDEFINED) MPI_Count *true_lb,
    _mpi_out_(true_extent, MPI_UNDEFINED) MPI_Count *true_extent
    );

MPI_METHOD
PMPI_Type_get_true_extent_x(
    _In_ MPI_Datatype datatype,
    _mpi_out_(true_lb, MPI_UNDEFINED) MPI_Count *true_lb,
    _mpi_out_(true_extent, MPI_UNDEFINED) MPI_Count *true_extent
    );


/*---------------------------------------------*/
/* Section 4.1.9: Datatype Commit and Free     */
/*---------------------------------------------*/

MPI_METHOD
MPI_Type_commit(
    _In_ MPI_Datatype* datatype
    );

MPI_METHOD
PMPI_Type_commit(
    _In_ MPI_Datatype* datatype
    );

MPI_METHOD
MPI_Type_free(
    _Deref_out_range_(==, MPI_DATATYPE_NULL) _Inout_ MPI_Datatype* datatype
    );

MPI_METHOD
PMPI_Type_free(
    _Deref_out_range_(==, MPI_DATATYPE_NULL) _Inout_ MPI_Datatype* datatype
    );


/*---------------------------------------------*/
/* Section 4.1.10: Datatype Duplication        */
/*---------------------------------------------*/

MPI_METHOD
MPI_Type_dup(
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_dup(
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );


/*---------------------------------------------*/
/* Section 4.1.11: Datatype and Communication  */
/*---------------------------------------------*/

MPI_METHOD
MPI_Get_elements(
    _In_ const MPI_Status* status,
    _In_ MPI_Datatype datatype,
    _mpi_out_(count, MPI_UNDEFINED) int* count
    );

MPI_METHOD
PMPI_Get_elements(
    _In_ const MPI_Status* status,
    _In_ MPI_Datatype datatype,
    _mpi_out_(count, MPI_UNDEFINED) int* count
    );

MPI_METHOD
MPI_Get_elements_x(
    _In_ const MPI_Status* status,
    _In_ MPI_Datatype datatype,
    _mpi_out_(count, MPI_UNDEFINED) MPI_Count *count
    );

MPI_METHOD
PMPI_Get_elements_x(
    _In_ const MPI_Status* status,
    _In_ MPI_Datatype datatype,
    _mpi_out_(count, MPI_UNDEFINED) MPI_Count *count
    );


/*---------------------------------------------*/
/* Section 4.1.13: Decoding a Datatype         */
/*---------------------------------------------*/

/* Datatype combiners result */
enum
{
    MPI_COMBINER_NAMED            = 1,
    MPI_COMBINER_DUP              = 2,
    MPI_COMBINER_CONTIGUOUS       = 3,
    MPI_COMBINER_VECTOR           = 4,
    MPI_COMBINER_HVECTOR_INTEGER  = 5,
    MPI_COMBINER_HVECTOR          = 6,
    MPI_COMBINER_INDEXED          = 7,
    MPI_COMBINER_HINDEXED_INTEGER = 8,
    MPI_COMBINER_HINDEXED         = 9,
    MPI_COMBINER_INDEXED_BLOCK    = 10,
    MPI_COMBINER_STRUCT_INTEGER   = 11,
    MPI_COMBINER_STRUCT           = 12,
    MPI_COMBINER_SUBARRAY         = 13,
    MPI_COMBINER_DARRAY           = 14,
    MPI_COMBINER_F90_REAL         = 15,
    MPI_COMBINER_F90_COMPLEX      = 16,
    MPI_COMBINER_F90_INTEGER      = 17,
    MPI_COMBINER_RESIZED          = 18,
    MPI_COMBINER_HINDEXED_BLOCK   = 19
};

MPI_METHOD
MPI_Type_get_envelope(
    _In_ MPI_Datatype datatype,
    _Out_ _Deref_out_range_(>=, 0) int* num_integers,
    _Out_ _Deref_out_range_(>=, 0) int* num_addresses,
    _Out_ _Deref_out_range_(>=, 0) int* num_datatypes,
    _Out_ _Deref_out_range_(MPI_COMBINER_NAMED, MPI_COMBINER_RESIZED) int* combiner
    );

MPI_METHOD
PMPI_Type_get_envelope(
    _In_ MPI_Datatype datatype,
    _Out_ _Deref_out_range_(>=, 0) int* num_integers,
    _Out_ _Deref_out_range_(>=, 0) int* num_addresses,
    _Out_ _Deref_out_range_(>=, 0) int* num_datatypes,
    _Out_ _Deref_out_range_(MPI_COMBINER_NAMED, MPI_COMBINER_RESIZED) int* combiner
    );

MPI_METHOD
MPI_Type_get_contents(
    _In_ MPI_Datatype datatype,
    _In_range_(>=, 0) int max_integers,
    _In_range_(>=, 0) int max_addresses,
    _In_range_(>=, 0) int max_datatypes,
    _mpi_writes_(max_integers) int array_of_integers[],
    _mpi_writes_(max_addresses) MPI_Aint array_of_addresses[],
    _mpi_writes_(max_datatypes) MPI_Datatype array_of_datatypes[]
    );

MPI_METHOD
PMPI_Type_get_contents(
    _In_ MPI_Datatype datatype,
    _In_range_(>=, 0) int max_integers,
    _In_range_(>=, 0) int max_addresses,
    _In_range_(>=, 0) int max_datatypes,
    _mpi_writes_(max_integers) int array_of_integers[],
    _mpi_writes_(max_addresses) MPI_Aint array_of_addresses[],
    _mpi_writes_(max_datatypes) MPI_Datatype array_of_datatypes[]
    );


/*---------------------------------------------*/
/* Section 4.2: Datatype Pack and Unpack       */
/*---------------------------------------------*/

MPI_METHOD
MPI_Pack(
    _In_opt_ const void* inbuf,
    _In_range_(>=, 0) int incount,
    _In_ MPI_Datatype datatype,
    _mpi_writes_bytes_(outsize) void* outbuf,
    _In_range_(>=, 0) int outsize,
    _mpi_position_(outsize) int* position,
    _In_ MPI_Comm comm
    );

MPI_METHOD
PMPI_Pack(
    _In_opt_ const void* inbuf,
    _In_range_(>=, 0) int incount,
    _In_ MPI_Datatype datatype,
    _mpi_writes_bytes_(outsize) void* outbuf,
    _In_range_(>=, 0) int outsize,
    _mpi_position_(outsize) int* position,
    _In_ MPI_Comm comm
    );

MPI_METHOD
MPI_Unpack(
    _mpi_reads_bytes_(insize) const void* inbuf,
    _In_range_(>=, 0) int insize,
    _mpi_position_(insize) int* position,
    _When_(insize > 0, _Out_opt_) void* outbuf,
    _In_range_(>=, 0) int outcount,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Comm comm
    );

MPI_METHOD
PMPI_Unpack(
    _mpi_reads_bytes_(insize) const void* inbuf,
    _In_range_(>=, 0) int insize,
    _mpi_position_(insize) int* position,
    _When_(insize > 0, _Out_opt_) void* outbuf,
    _In_range_(>=, 0) int outcount,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Comm comm
    );

MPI_METHOD
MPI_Pack_size(
    _In_range_(>=, 0) int incount,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Comm comm,
    _mpi_out_(size, MPI_UNDEFINED) int *size
    );

MPI_METHOD
PMPI_Pack_size(
    _In_range_(>=, 0) int incount,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Comm comm,
    _mpi_out_(size, MPI_UNDEFINED) int *size
    );


/*---------------------------------------------*/
/* Section 4.3: Canonical Pack and Unpack      */
/*---------------------------------------------*/

MPI_METHOD
MPI_Pack_external(
    _In_z_ const char* datarep,
    _In_opt_ const void* inbuf,
    _In_range_(>=, 0) int incount,
    _In_ MPI_Datatype datatype,
    _mpi_writes_bytes_(outsize) void* outbuf,
    _In_range_(>=, 0) MPI_Aint outsize,
    _mpi_position_(outsize) MPI_Aint* position
    );

MPI_METHOD
PMPI_Pack_external(
    _In_z_ const char* datarep,
    _In_opt_ const void* inbuf,
    _In_range_(>=, 0) int incount,
    _In_ MPI_Datatype datatype,
    _mpi_writes_bytes_(outsize) void* outbuf,
    _In_range_(>=, 0) MPI_Aint outsize,
    _mpi_position_(outsize) MPI_Aint* position
    );

MPI_METHOD
MPI_Unpack_external(
    _In_z_ const char* datarep,
    _In_reads_bytes_opt_(insize) const void* inbuf,
    _In_range_(>=, 0) MPI_Aint insize,
    _mpi_position_(insize) MPI_Aint* position,
    _When_(insize > 0, _Out_opt_) void* outbuf,
    _In_range_(>=, 0) int outcount,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
PMPI_Unpack_external(
    _In_z_ const char* datarep,
    _In_reads_bytes_opt_(insize) const void* inbuf,
    _In_range_(>=, 0) MPI_Aint insize,
    _mpi_position_(insize) MPI_Aint* position,
    _When_(insize > 0, _Out_opt_) void* outbuf,
    _In_range_(>=, 0) int outcount,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
MPI_Pack_external_size(
    _In_z_ const char* datarep,
    _In_range_(>=, 0) int incount,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Aint* size
    );

MPI_METHOD
PMPI_Pack_external_size(
    _In_z_ const char* datarep,
    _In_range_(>=, 0) int incount,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Aint* size
    );


/*---------------------------------------------------------------------------*/
/* Chapter 5: Collective Communication                                       */
/*---------------------------------------------------------------------------*/

#define MPI_IN_PLACE ((void*)(MPI_Aint)-1)

/*---------------------------------------------*/
/* Section 5.3: Barrier Synchronization        */
/*---------------------------------------------*/

MPI_METHOD
MPI_Barrier(
    _In_ MPI_Comm comm
    );

MPI_METHOD
PMPI_Barrier(
    _In_ MPI_Comm comm
    );


/*---------------------------------------------*/
/* Section 5.4: Broadcast                      */
/*---------------------------------------------*/

MPI_METHOD
MPI_Bcast(
    _Pre_opt_valid_ void* buffer,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );

MPI_METHOD
PMPI_Bcast(
    _Pre_opt_valid_ void* buffer,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );


/*---------------------------------------------*/
/* Section 5.5: Gather                         */
/*---------------------------------------------*/

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Gather(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Gather(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Gatherv(
    _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_opt_ const int recvcounts[],
    _In_opt_ const int displs[],
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Gatherv(
    _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_opt_ const int recvcounts[],
    _In_opt_ const int displs[],
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );


/*---------------------------------------------*/
/* Section 5.6: Scatter                        */
/*---------------------------------------------*/

_Pre_satisfies_(sendbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Scatter(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(sendbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Scatter(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(sendbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Scatterv(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int displs[],
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(sendbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Scatterv(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int displs[],
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );


/*---------------------------------------------*/
/* Section 5.6: Gather-to-all                  */
/*---------------------------------------------*/

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Allgather(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Allgather(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Allgatherv(
    _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int displs[],
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Allgatherv(
    _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int displs[],
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm
    );


/*---------------------------------------------*/
/* Section 5.6: All-to-All Scatter/Gather      */
/*---------------------------------------------*/

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Alltoall(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Alltoall(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Alltoallv(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int sdispls[],
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int rdispls[],
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Alltoallv(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int sdispls[],
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int rdispls[],
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Alltoallw(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int sdispls[],
    _In_opt_ const MPI_Datatype sendtypes[],
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int rdispls[],
    _In_ const MPI_Datatype recvtypes[],
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Alltoallw(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int sdispls[],
    _In_opt_ const MPI_Datatype sendtypes[],
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int rdispls[],
    _In_ const MPI_Datatype recvtypes[],
    _In_ MPI_Comm comm
    );


/*---------------------------------------------*/
/* Section 5.9: Global Reduction Operations    */
/*---------------------------------------------*/

typedef
void
(MPIAPI MPI_User_function)(
    _In_opt_ void* invec,
    _Inout_opt_ void* inoutvec,
    _In_ int* len,
    _In_ MPI_Datatype* datatype
    );

MPI_METHOD
MPI_Op_commutative(
    _In_ MPI_Op op,
    _Out_ int* commute
    );

MPI_METHOD
PMPI_Op_commutative(
    _In_ MPI_Op op,
    _Out_ int* commute
    );

MPI_METHOD
MPI_Op_create(
    _In_ MPI_User_function* user_fn,
    _In_ int commute,
    _Out_ MPI_Op* op
    );

MPI_METHOD
PMPI_Op_create(
    _In_ MPI_User_function* user_fn,
    _In_ int commute,
    _Out_ MPI_Op* op
    );

MPI_METHOD
MPI_Op_free(
    _Inout_ MPI_Op* op
    );

MPI_METHOD
PMPI_Op_free(
    _Inout_ MPI_Op* op
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Reduce(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Reduce(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Allreduce(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Allreduce(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(inbuf != MPI_IN_PLACE)
_Pre_satisfies_(inoutbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Reduce_local(
    _In_opt_ _In_range_(!=, inoutbuf) const void *inbuf,
    _Inout_opt_ void *inoutbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op
    );

_Pre_satisfies_(inbuf != MPI_IN_PLACE)
_Pre_satisfies_(inoutbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Reduce_local(
    _In_opt_ _In_range_(!=, inoutbuf) const void *inbuf,
    _Inout_opt_ void *inoutbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op
    );

/*---------------------------------------------*/
/* Section 5.10: Reduce-Scatter                */
/*---------------------------------------------*/

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Reduce_scatter_block(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=,0) int recvcount,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Reduce_scatter_block(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Reduce_scatter(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Reduce_scatter(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm
    );


/*---------------------------------------------*/
/* Section 5.11: Scan                          */
/*---------------------------------------------*/

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Scan(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Scan(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Exscan(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Exscan(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm
    );


//
// Section 5.12: Nonblocking Collective Operations
//
_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Iallgather(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Iallgather(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Iallgatherv(
    _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int displs[],
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Iallgatherv(
    _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int displs[],
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Iallreduce(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Iallreduce(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Ialltoall(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Ialltoall(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Ialltoallv(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int sdispls[],
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int rdispls[],
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Ialltoallv(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int sdispls[],
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int rdispls[],
    _In_ MPI_Datatype recvtype,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Ialltoallw(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int sdispls[],
    _In_opt_ const MPI_Datatype sendtypes[],
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int rdispls[],
    _In_ const MPI_Datatype recvtypes[],
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Ialltoallw(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int sdispls[],
    _In_opt_ const MPI_Datatype sendtypes[],
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ const int rdispls[],
    _In_ const MPI_Datatype recvtypes[],
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_Ibarrier(
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Ibarrier(
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_Ibcast(
    _Pre_opt_valid_ void* buffer,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Ibcast(
    _Pre_opt_valid_ void* buffer,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Iexscan(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Iexscan(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Igather(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Igather(
    _In_opt_ _When_(sendtype == recvtype, _In_range_(!=, recvbuf)) const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Igatherv(
    _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_opt_ const int recvcounts[],
    _In_opt_ const int displs[],
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Igatherv(
    _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _Out_opt_ void* recvbuf,
    _In_opt_ const int recvcounts[],
    _In_opt_ const int displs[],
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Ireduce(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Ireduce(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Ireduce_scatter(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Ireduce_scatter(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_ const int recvcounts[],
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Iscan(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Iscan(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Ireduce_scatter_block(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=,0) int recvcount,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(recvbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Ireduce_scatter_block(
    _In_opt_ _In_range_(!=, recvbuf) const void* sendbuf,
    _Out_opt_ void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype datatype,
    _In_ MPI_Op op,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(sendbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Iscatter(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(sendbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Iscatter(
    _In_range_(!=, recvbuf) _In_opt_ const void* sendbuf,
    _In_range_(>=, 0) int sendcount,
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(sendbuf != MPI_IN_PLACE)
MPI_METHOD
MPI_Iscatterv(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int displs[],
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

_Pre_satisfies_(sendbuf != MPI_IN_PLACE)
MPI_METHOD
PMPI_Iscatterv(
    _In_opt_ const void* sendbuf,
    _In_opt_ const int sendcounts[],
    _In_opt_ const int displs[],
    _In_ MPI_Datatype sendtype,
    _When_(root != MPI_PROC_NULL, _Out_opt_) void* recvbuf,
    _In_range_(>=, 0) int recvcount,
    _In_ MPI_Datatype recvtype,
    _mpi_coll_rank_(root) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Request* request
    );

/*---------------------------------------------------------------------------*/
/* Chapter 6: Groups, Contexts, Communicators, and Caching                   */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------*/
/* Section 6.3: Group Management               */
/*---------------------------------------------*/

MPI_METHOD
MPI_Group_size(
    _In_ MPI_Group group,
    _Out_ _Deref_out_range_(>, 0) int* size
    );

MPI_METHOD
PMPI_Group_size(
    _In_ MPI_Group group,
    _Out_ _Deref_out_range_(>, 0) int* size
    );

MPI_METHOD
MPI_Group_rank(
    _In_ MPI_Group group,
    _Out_ _Deref_out_range_(>=, MPI_UNDEFINED) int* rank
    );

MPI_METHOD
PMPI_Group_rank(
    _In_ MPI_Group group,
    _Out_ _Deref_out_range_(>=, MPI_UNDEFINED) int* rank
    );

MPI_METHOD
MPI_Group_translate_ranks(
    _In_ MPI_Group group1,
    _In_ int n,
    _In_reads_opt_(n) const int ranks1[],
    _In_ MPI_Group group2,
    _Out_writes_opt_(n) int ranks2[]
    );

MPI_METHOD
PMPI_Group_translate_ranks(
    _In_ MPI_Group group1,
    _In_ int n,
    _In_reads_opt_(n) const int ranks1[],
    _In_ MPI_Group group2,
    _Out_writes_opt_(n) int ranks2[]
    );

/* Results of the compare operations */
#define MPI_IDENT       0
#define MPI_CONGRUENT   1
#define MPI_SIMILAR     2
#define MPI_UNEQUAL     3

MPI_METHOD
MPI_Group_compare(
    _In_ MPI_Group group1,
    _In_ MPI_Group group2,
    _Out_ int* result
    );

MPI_METHOD
PMPI_Group_compare(
    _In_ MPI_Group group1,
    _In_ MPI_Group group2,
    _Out_ int* result
    );

MPI_METHOD
MPI_Comm_group(
    _In_ MPI_Comm comm,
    _Out_ MPI_Group* group
    );

MPI_METHOD
PMPI_Comm_group(
    _In_ MPI_Comm comm,
    _Out_ MPI_Group* group
    );

MPI_METHOD
MPI_Group_union(
    _In_ MPI_Group group1,
    _In_ MPI_Group group2,
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
PMPI_Group_union(
    _In_ MPI_Group group1,
    _In_ MPI_Group group2,
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
MPI_Group_intersection(
    _In_ MPI_Group group1,
    _In_ MPI_Group group2,
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
PMPI_Group_intersection(
   _In_  MPI_Group group1,
    _In_ MPI_Group group2,
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
MPI_Group_difference(
    _In_ MPI_Group group1,
    _In_ MPI_Group group2,
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
PMPI_Group_difference(
    _In_ MPI_Group group1,
    _In_ MPI_Group group2,
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
MPI_Group_incl(
    _In_ MPI_Group group,
    _In_range_(>=, 0) int n,
    _In_reads_opt_(n) const int ranks[],
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
PMPI_Group_incl(
    _In_ MPI_Group group,
    _In_range_(>=, 0) int n,
    _In_reads_opt_(n) const int ranks[],
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
MPI_Group_excl(
    _In_ MPI_Group group,
    _In_range_(>=, 0) int n,
    _In_reads_opt_(n) const int ranks[],
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
PMPI_Group_excl(
    _In_ MPI_Group group,
    _In_range_(>=, 0) int n,
    _In_reads_opt_(n) const int ranks[],
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
MPI_Group_range_incl(
    _In_ MPI_Group group,
    _In_range_(>=, 0) int n,
    _In_reads_opt_(n) int ranges[][3],
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
PMPI_Group_range_incl(
    _In_ MPI_Group group,
    _In_range_(>=, 0) int n,
    _In_reads_opt_(n) int ranges[][3],
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
MPI_Group_range_excl(
    _In_ MPI_Group group,
    _In_range_(>=, 0) int n,
    _In_reads_opt_(n) int ranges[][3],
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
PMPI_Group_range_excl(
    _In_ MPI_Group group,
    _In_range_(>=, 0) int n,
    _In_reads_opt_(n) int ranges[][3],
    _Out_ MPI_Group* newgroup
    );

MPI_METHOD
MPI_Group_free(
    _Inout_ MPI_Group* group
    );

MPI_METHOD
PMPI_Group_free(
    _Inout_ MPI_Group* group
    );


/*---------------------------------------------*/
/* Section 6.4: Communicator Management        */
/*---------------------------------------------*/

MPI_METHOD
MPI_Comm_size(
    _In_ MPI_Comm comm,
    _Out_ _Deref_out_range_(>, 0) int* size
    );

MPI_METHOD
PMPI_Comm_size(
    _In_ MPI_Comm comm,
    _Out_ _Deref_out_range_(>, 0) int* size
    );

MPI_METHOD
MPI_Comm_rank(
    _In_ MPI_Comm comm,
    _Out_ _Deref_out_range_(>=, 0)  int* rank
    );

MPI_METHOD
PMPI_Comm_rank(
    _In_ MPI_Comm comm,
    _Out_ _Deref_out_range_(>=, 0)  int* rank
    );

MPI_METHOD
MPI_Comm_compare(
    _In_ MPI_Comm comm1,
    _In_ MPI_Comm comm2,
    _Out_ int* result
    );

MPI_METHOD
PMPI_Comm_compare(
    _In_ MPI_Comm comm1,
    _In_ MPI_Comm comm2,
    _Out_ int* result
    );

MPI_METHOD
MPI_Comm_dup(
    _In_ MPI_Comm comm,
    _Out_ MPI_Comm* newcomm
    );

MPI_METHOD
PMPI_Comm_dup(
    _In_ MPI_Comm comm,
    _Out_ MPI_Comm* newcomm
    );

MPI_METHOD
MPI_Comm_create(
    _In_ MPI_Comm comm,
    _In_ MPI_Group group,
    _Out_ MPI_Comm* newcomm
    );

MPI_METHOD
PMPI_Comm_create(
    _In_ MPI_Comm comm,
    _In_ MPI_Group group,
    _Out_ MPI_Comm* newcomm
    );

MPI_METHOD
MPI_Comm_split(
    _In_ MPI_Comm comm,
    _In_ int color,
    _In_ int key,
    _Out_ MPI_Comm* newcomm
    );

MPI_METHOD
PMPI_Comm_split(
    _In_ MPI_Comm comm,
    _In_ int color,
    _In_ int key,
    _Out_ MPI_Comm* newcomm
    );

MPI_METHOD
MPI_Comm_split_type(
    _In_ MPI_Comm comm,
    _In_ int split_type,
    _In_ int key,
    _In_ MPI_Info info,
    _Out_ MPI_Comm *newcomm
    );

MPI_METHOD
PMPI_Comm_split_type(
    _In_ MPI_Comm comm,
    _In_ int split_type,
    _In_ int key,
    _In_ MPI_Info info,
    _Out_ MPI_Comm *newcomm
    );

MPI_METHOD
MPI_Comm_free(
    _Inout_ MPI_Comm* comm
    );

MPI_METHOD
PMPI_Comm_free(
    _Inout_ MPI_Comm* comm
    );


/*---------------------------------------------*/
/* Section 6.6: Inter-Communication            */
/*---------------------------------------------*/

MPI_METHOD
MPI_Comm_test_inter(
    _In_ MPI_Comm comm,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
PMPI_Comm_test_inter(
    _In_ MPI_Comm comm,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
MPI_Comm_remote_size(
    _In_ MPI_Comm comm,
    _Out_ int* size
    );

MPI_METHOD
PMPI_Comm_remote_size(
    _In_ MPI_Comm comm,
    _Out_ int* size
    );

MPI_METHOD
MPI_Comm_remote_group(
    _In_ MPI_Comm comm,
    _Out_ MPI_Group* group
    );

MPI_METHOD
PMPI_Comm_remote_group(
    _In_ MPI_Comm comm,
    _Out_ MPI_Group* group
    );

MPI_METHOD
MPI_Intercomm_create(
    _In_ MPI_Comm local_comm,
    _In_range_(>=, 0) int local_leader,
    _In_ MPI_Comm peer_comm,
    _In_range_(>=, 0) int remote_leader,
    _In_range_(>=, 0) int tag,
    _Out_ MPI_Comm* newintercomm
    );

MPI_METHOD
PMPI_Intercomm_create(
    _In_ MPI_Comm local_comm,
    _In_range_(>=, 0) int local_leader,
    _In_ MPI_Comm peer_comm,
    _In_range_(>=, 0) int remote_leader,
    _In_range_(>=, 0) int tag,
    _Out_ MPI_Comm* newintercomm
    );

MPI_METHOD
MPI_Intercomm_merge(
    _In_ MPI_Comm intercomm,
    _In_ int high,
    _Out_ MPI_Comm* newintracomm
    );

MPI_METHOD
PMPI_Intercomm_merge(
    _In_ MPI_Comm intercomm,
    _In_ int high,
    _Out_ MPI_Comm* newintracomm
    );


/*---------------------------------------------*/
/* Section 6.7: Caching                        */
/*---------------------------------------------*/

#define MPI_KEYVAL_INVALID  0x24000000

typedef
int
(MPIAPI MPI_Comm_copy_attr_function)(
    _In_ MPI_Comm oldcomm,
    _In_ int comm_keyval,
    _In_opt_ void* extra_state,
    _In_opt_ void* attribute_val_in,
    _Out_ void* attribute_val_out,
    _mpi_out_flag_ int* flag
    );

typedef
int
(MPIAPI MPI_Comm_delete_attr_function)(
    _In_ MPI_Comm comm,
    _In_ int comm_keyval,
    _In_opt_ void* attribute_val,
    _In_opt_ void* extra_state
    );

#define MPI_COMM_NULL_COPY_FN ((MPI_Comm_copy_attr_function*)0)
#define MPI_COMM_NULL_DELETE_FN ((MPI_Comm_delete_attr_function*)0)
#define MPI_COMM_DUP_FN ((MPI_Comm_copy_attr_function*)MPIR_Dup_fn)

MPI_METHOD
MPI_Comm_create_keyval(
    _In_opt_ MPI_Comm_copy_attr_function* comm_copy_attr_fn,
    _In_opt_ MPI_Comm_delete_attr_function* comm_delete_attr_fn,
    _Out_ int* comm_keyval,
    _In_opt_ void* extra_state
    );

MPI_METHOD
PMPI_Comm_create_keyval(
    _In_opt_ MPI_Comm_copy_attr_function* comm_copy_attr_fn,
    _In_opt_ MPI_Comm_delete_attr_function* comm_delete_attr_fn,
    _Out_ int* comm_keyval,
    _In_opt_ void* extra_state
    );

MPI_METHOD
MPI_Comm_free_keyval(
    _Inout_ int* comm_keyval
    );

MPI_METHOD
PMPI_Comm_free_keyval(
    _Inout_ int* comm_keyval
    );

MPI_METHOD
MPI_Comm_set_attr(
    _In_ MPI_Comm comm,
    _In_ int comm_keyval,
    _In_opt_ void* attribute_val
    );

MPI_METHOD
PMPI_Comm_set_attr(
    _In_ MPI_Comm comm,
    _In_ int comm_keyval,
    _In_opt_ void* attribute_val
    );


/* Predefined comm attribute key values */
/* C Versions (return pointer to value),
   Fortran Versions (return integer value).

   DO NOT CHANGE THESE.  The values encode:
   builtin kind (0x1 in bit 30-31)
   Keyval object (0x9 in bits 26-29)
   for communicator (0x1 in bits 22-25)

   Fortran versions of the attributes are formed by adding one to
   the C version.
 */
#define MPI_TAG_UB          0x64400001
#define MPI_HOST            0x64400003
#define MPI_IO              0x64400005
#define MPI_WTIME_IS_GLOBAL 0x64400007
#define MPI_UNIVERSE_SIZE   0x64400009
#define MPI_LASTUSEDCODE    0x6440000b
#define MPI_APPNUM          0x6440000d

MPI_METHOD
MPI_Comm_get_attr(
    _In_ MPI_Comm comm,
    _In_ int comm_keyval,
    _When_(*flag != 0, _Out_) void* attribute_val,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
PMPI_Comm_get_attr(
    _In_ MPI_Comm comm,
    _In_ int comm_keyval,
    _When_(*flag != 0, _Out_) void* attribute_val,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
MPI_Comm_delete_attr(
    _In_ MPI_Comm comm,
    _In_ int comm_keyval
    );

MPI_METHOD
PMPI_Comm_delete_attr(
    _In_ MPI_Comm comm,
    _In_ int comm_keyval
    );


typedef
int
(MPIAPI MPI_Win_copy_attr_function)(
    _In_ MPI_Win oldwin,
    _In_ int win_keyval,
    _In_opt_ void* extra_state,
    _In_opt_ void* attribute_val_in,
    _Out_ void* attribute_val_out,
    _mpi_out_flag_ int* flag
    );

typedef
int
(MPIAPI MPI_Win_delete_attr_function)(
    _In_ MPI_Win win,
    _In_ int win_keyval,
    _In_opt_ void* attribute_val,
    _In_opt_ void* extra_state
    );

#define MPI_WIN_NULL_COPY_FN ((MPI_Win_copy_attr_function*)0)
#define MPI_WIN_NULL_DELETE_FN ((MPI_Win_delete_attr_function*)0)
#define MPI_WIN_DUP_FN ((MPI_Win_copy_attr_function*)MPIR_Dup_fn)

MPI_METHOD
MPI_Win_create_keyval(
    _In_opt_ MPI_Win_copy_attr_function* win_copy_attr_fn,
    _In_opt_ MPI_Win_delete_attr_function* win_delete_attr_fn,
    _Out_ int* win_keyval,
    _In_opt_ void* extra_state
    );

MPI_METHOD
PMPI_Win_create_keyval(
    _In_opt_ MPI_Win_copy_attr_function* win_copy_attr_fn,
    _In_opt_ MPI_Win_delete_attr_function* win_delete_attr_fn,
    _Out_ int* win_keyval,
    _In_opt_ void* extra_state
    );

MPI_METHOD
MPI_Win_free_keyval(
    _Inout_ int* win_keyval
    );

MPI_METHOD
PMPI_Win_free_keyval(
    _Inout_ int* win_keyval
    );

MPI_METHOD
MPI_Win_set_attr(
    _In_ MPI_Win win,
    _In_ int win_keyval,
    _In_opt_ void* attribute_val
    );

MPI_METHOD
PMPI_Win_set_attr(
    _In_ MPI_Win win,
    _In_ int win_keyval,
    _In_opt_ void* attribute_val
    );


/* Predefined window key value attributes */
#define MPI_WIN_BASE              0x66000001
#define MPI_WIN_SIZE              0x66000003
#define MPI_WIN_DISP_UNIT         0x66000005
#define MPI_WIN_CREATE_FLAVOR     0x66000007
#define MPI_WIN_MODEL             0x66000009

/* MPI Window Create Flavors */
#define MPI_WIN_FLAVOR_CREATE     1
#define MPI_WIN_FLAVOR_ALLOCATE   2
#define MPI_WIN_FLAVOR_DYNAMIC    3
#define MPI_WIN_FLAVOR_SHARED     4

/* MPI Window Models */
#define MPI_WIN_SEPARATE          1
#define MPI_WIN_UNIFIED           2

MPI_METHOD
MPI_Win_get_attr(
    _In_ MPI_Win win,
    _In_ int win_keyval,
    _When_(*flag != 0, _Out_) void* attribute_val,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
PMPI_Win_get_attr(
    _In_ MPI_Win win,
    _In_ int win_keyval,
    _When_(*flag != 0, _Out_) void* attribute_val,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
MPI_Win_delete_attr(
    _In_ MPI_Win win,
    _In_ int win_keyval
    );

MPI_METHOD
PMPI_Win_delete_attr(
    _In_ MPI_Win win,
    _In_ int win_keyval
    );


typedef
int
(MPIAPI MPI_Type_copy_attr_function)(
    MPI_Datatype olddatatype,
    int datatype_keyval,
    _In_opt_ void* extra_state,
    _In_opt_ void* attribute_val_in,
    _Out_ void* attribute_val_out,
    _mpi_out_flag_ int* flag
    );

typedef
int
(MPIAPI MPI_Type_delete_attr_function)(
    MPI_Datatype datatype,
    int datatype_keyval,
    _In_opt_ void* attribute_val,
    _In_opt_ void* extra_state
    );

#define MPI_TYPE_NULL_COPY_FN ((MPI_Type_copy_attr_function*)0)
#define MPI_TYPE_NULL_DELETE_FN ((MPI_Type_delete_attr_function*)0)
#define MPI_TYPE_DUP_FN ((MPI_Type_copy_attr_function*)MPIR_Dup_fn)

MPI_METHOD
MPI_Type_create_keyval(
    _In_opt_ MPI_Type_copy_attr_function* type_copy_attr_fn,
    _In_opt_ MPI_Type_delete_attr_function* type_delete_attr_fn,
    _Out_ int* type_keyval,
    _In_opt_ void* extra_state
    );

MPI_METHOD
PMPI_Type_create_keyval(
    _In_opt_ MPI_Type_copy_attr_function* type_copy_attr_fn,
    _In_opt_ MPI_Type_delete_attr_function* type_delete_attr_fn,
    _Out_ int* type_keyval,
    _In_opt_ void* extra_state
    );

MPI_METHOD
MPI_Type_free_keyval(
    _Inout_ int* type_keyval
    );

MPI_METHOD
PMPI_Type_free_keyval(
    _Inout_ int* type_keyval
    );

MPI_METHOD
MPI_Type_set_attr(
    _In_ MPI_Datatype type,
    _In_ int type_keyval,
    _In_opt_ void* attribute_val
    );

MPI_METHOD
PMPI_Type_set_attr(
    _In_ MPI_Datatype type,
    _In_ int type_keyval,
    _In_opt_ void* attribute_val
    );

MPI_METHOD
MPI_Type_get_attr(
    _In_ MPI_Datatype type,
    _In_ int type_keyval,
    _When_(*flag != 0, _Out_) void* attribute_val,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
PMPI_Type_get_attr(
    _In_ MPI_Datatype type,
    _In_ int type_keyval,
    _When_(*flag != 0, _Out_) void* attribute_val,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
MPI_Type_delete_attr(
    _In_ MPI_Datatype type,
    _In_ int type_keyval
    );

MPI_METHOD
PMPI_Type_delete_attr(
    _In_ MPI_Datatype type,
    _In_ int type_keyval
    );


/*---------------------------------------------*/
/* Section 6.8: Naming Objects                 */
/*---------------------------------------------*/

#define MPI_MAX_OBJECT_NAME 128

MPI_METHOD
MPI_Comm_set_name(
    _In_ MPI_Comm comm,
    _In_z_ const char* comm_name
    );

MPI_METHOD
PMPI_Comm_set_name(
    _In_ MPI_Comm comm,
    _In_z_ const char* comm_name
    );

MPI_METHOD
MPI_Comm_get_name(
    _In_ MPI_Comm comm,
    _Out_writes_z_(MPI_MAX_OBJECT_NAME) char* comm_name,
    _Out_ int* resultlen
    );

MPI_METHOD
PMPI_Comm_get_name(
    _In_ MPI_Comm comm,
    _Out_writes_z_(MPI_MAX_OBJECT_NAME) char* comm_name,
    _Out_ int* resultlen
    );

MPI_METHOD
MPI_Type_set_name(
    _In_ MPI_Datatype datatype,
    _In_z_ const char* type_name
    );

MPI_METHOD
PMPI_Type_set_name(
    _In_ MPI_Datatype datatype,
    _In_z_ const char* type_name
    );

MPI_METHOD
MPI_Type_get_name(
    _In_ MPI_Datatype datatype,
    _Out_writes_z_(MPI_MAX_OBJECT_NAME) char* type_name,
    _Out_ int* resultlen
    );

MPI_METHOD
PMPI_Type_get_name(
    _In_ MPI_Datatype datatype,
    _Out_writes_z_(MPI_MAX_OBJECT_NAME) char* type_name,
    _Out_ int* resultlen
    );

MPI_METHOD
MPI_Win_set_name(
    _In_ MPI_Win win,
    _In_z_ const char* win_name
    );

MPI_METHOD
PMPI_Win_set_name(
    _In_ MPI_Win win,
    _In_z_ const char* win_name
    );

MPI_METHOD
MPI_Win_get_name(
    _In_ MPI_Win win,
    _Out_writes_z_(MPI_MAX_OBJECT_NAME) char* win_name,
    _Out_ int* resultlen
    );

MPI_METHOD
PMPI_Win_get_name(
    _In_ MPI_Win win,
    _Out_writes_z_(MPI_MAX_OBJECT_NAME) char* win_name,
    _Out_ int* resultlen
    );


/*---------------------------------------------------------------------------*/
/* Chapter 7: Process Topologies                                             */
/*---------------------------------------------------------------------------*/

MPI_METHOD
MPI_Cart_create(
    _In_ MPI_Comm comm_old,
    _In_range_(>=, 0) int ndims,
    _In_reads_opt_(ndims) const int dims[],
    _In_reads_opt_(ndims) const int periods[],
    _In_ int reorder,
    _Out_ MPI_Comm* comm_cart
    );

MPI_METHOD
PMPI_Cart_create(
    _In_ MPI_Comm comm_old,
    _In_range_(>=, 0) int ndims,
    _In_reads_opt_(ndims) const int dims[],
    _In_reads_opt_(ndims) const int periods[],
    _In_ int reorder,
    _Out_ MPI_Comm* comm_cart
    );

MPI_METHOD
MPI_Dims_create(
    _In_range_(>, 0) int nnodes,
    _In_range_(>=, 0) int ndims,
    _Inout_updates_opt_(ndims) int dims[]
    );

MPI_METHOD
PMPI_Dims_create(
    _In_range_(>, 0) int nnodes,
    _In_range_(>=, 0) int ndims,
    _Inout_updates_opt_(ndims) int dims[]
    );

MPI_METHOD
MPI_Graph_create(
    _In_ MPI_Comm comm_old,
    _In_range_(>=, 0) int nnodes,
    _In_reads_opt_(nnodes) const int index[],
    _In_reads_opt_(nnodes) const int edges[],
    _In_ int reorder,
    _Out_ MPI_Comm* comm_graph
    );

MPI_METHOD
PMPI_Graph_create(
    _In_ MPI_Comm comm_old,
    _In_range_(>=, 0) int nnodes,
    _In_reads_opt_(nnodes) const int index[],
    _In_opt_ const int edges[],
    _In_ int reorder,
    _Out_ MPI_Comm* comm_graph
    );

MPI_METHOD
MPI_Dist_graph_create_adjacent(
    _In_ MPI_Comm comm_old,
    _In_range_(>=, 0) int indegree,
    _In_reads_opt_(indegree) const int sources[],
    _In_reads_opt_(indegree) const int sourceweights[],
    _In_range_(>=, 0) int outdegree,
    _In_reads_opt_(outdegree) const int destinations[],
    _In_reads_opt_(outdegree) const int destweights[],
    _In_ MPI_Info info,
    _In_range_(0,1) int reorder,
    _Out_ MPI_Comm* comm_dist_graph
);

MPI_METHOD
PMPI_Dist_graph_create_adjacent(
    _In_ MPI_Comm comm_old,
    _In_range_(>=, 0) int indegree,
    _In_reads_opt_(indegree) const int sources[],
    _In_reads_opt_(indegree) const int sourceweights[],
    _In_range_(>=, 0) int outdegree,
    _In_reads_opt_(outdegree) const int destinations[],
    _In_reads_opt_(outdegree) const int destweights[],
    _In_ MPI_Info info,
    _In_range_(0,1) int reorder,
    _Out_ MPI_Comm* comm_dist_graph
);

MPI_METHOD
MPI_Dist_graph_create(
    _In_ MPI_Comm comm_old,
    _In_range_(>=, 0) int n,
    _In_reads_opt_(n) const int sources[],
    _In_reads_opt_(n) const int degrees[],
    _In_opt_ const int destinations[],
    _In_opt_ const int weights[],
    _In_ MPI_Info info,
    _In_range_(0, 1) int reorder,
    _Out_ MPI_Comm *comm_dist_graph
    );

MPI_METHOD
PMPI_Dist_graph_create(
    _In_ MPI_Comm comm_old,
    _In_range_(>=, 0) int n,
    _In_reads_opt_(n) const int sources[],
    _In_reads_opt_(n) const int degrees[],
    _In_opt_ const int destinations[],
    _In_opt_ const int weights[],
    _In_ MPI_Info info,
    _In_range_(0, 1) int reorder,
    _Out_ MPI_Comm *comm_dist_graph
    );

/* Topology types */
enum
{
    MPI_GRAPH      = 1,
    MPI_CART       = 2,
    MPI_DIST_GRAPH = 3
};

MPI_METHOD
MPI_Topo_test(
    _In_ MPI_Comm comm,
    _Out_ int* status
    );

MPI_METHOD
PMPI_Topo_test(
    _In_ MPI_Comm comm,
    _Out_ int* status
    );

MPI_METHOD
MPI_Graphdims_get(
   _In_  MPI_Comm comm,
    _Out_ int* nnodes,
    _Out_ int* nedges
    );

MPI_METHOD
PMPI_Graphdims_get(
    _In_ MPI_Comm comm,
    _Out_ int* nnodes,
    _Out_ int* nedges
    );

MPI_METHOD
MPI_Graph_get(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int maxindex,
    _In_range_(>=, 0) int maxedges,
    _Out_writes_opt_(maxindex) int index[],
    _Out_writes_opt_(maxedges) int edges[]
    );

MPI_METHOD
PMPI_Graph_get(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int maxindex,
    _In_range_(>=, 0) int maxedges,
    _Out_writes_opt_(maxindex) int index[],
    _Out_writes_opt_(maxedges) int edges[]
    );

MPI_METHOD
MPI_Cartdim_get(
    _In_ MPI_Comm comm,
    _Out_ int* ndims
    );

MPI_METHOD
PMPI_Cartdim_get(
    _In_ MPI_Comm comm,
    _Out_ int* ndims
    );

MPI_METHOD
MPI_Cart_get(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int maxdims,
    _Out_writes_opt_(maxdims) int dims[],
    _Out_writes_opt_(maxdims) int periods[],
    _Out_writes_opt_(maxdims) int coords[]
    );

MPI_METHOD
PMPI_Cart_get(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int maxdims,
    _Out_writes_opt_(maxdims) int dims[],
    _Out_writes_opt_(maxdims) int periods[],
    _Out_writes_opt_(maxdims) int coords[]
    );

MPI_METHOD
MPI_Cart_rank(
    _In_ MPI_Comm comm,
    _In_ const int coords[],
    _Out_ _Deref_out_range_(>=, 0) int* rank
    );

MPI_METHOD
PMPI_Cart_rank(
    _In_ MPI_Comm comm,
    _In_ const int coords[],
    _Out_ _Deref_out_range_(>=, 0) int* rank
    );

MPI_METHOD
MPI_Cart_coords(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int rank,
    _In_range_(>=, 0) int maxdims,
    _Out_writes_opt_(maxdims) int coords[]
    );

MPI_METHOD
PMPI_Cart_coords(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int rank,
    _In_range_(>=, 0) int maxdims,
    _Out_writes_opt_(maxdims) int coords[]
    );

MPI_METHOD
MPI_Graph_neighbors_count(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int rank,
    _Out_ _Deref_out_range_(>=, 0) int* nneighbors
    );

MPI_METHOD
PMPI_Graph_neighbors_count(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int rank,
    _Out_ _Deref_out_range_(>=, 0) int* nneighbors
    );

MPI_METHOD
MPI_Graph_neighbors(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int rank,
    _In_range_(>=, 0) int maxneighbors,
    _Out_writes_opt_(maxneighbors) int neighbors[]
    );

MPI_METHOD
PMPI_Graph_neighbors(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int rank,
    _In_range_(>=, 0) int maxneighbors,
    _Out_writes_opt_(maxneighbors) int neighbors[]
    );

MPI_METHOD
MPI_Cart_shift(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int direction,
    _In_ int disp,
    _Out_ _Deref_out_range_(>=, MPI_PROC_NULL) int* rank_source,
    _Out_ _Deref_out_range_(>=, MPI_PROC_NULL)  int* rank_dest
    );

MPI_METHOD
PMPI_Cart_shift(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int direction,
    _In_ int disp,
    _Out_ _Deref_out_range_(>=, MPI_PROC_NULL) int* rank_source,
    _Out_ _Deref_out_range_(>=, MPI_PROC_NULL)  int* rank_dest
    );

MPI_METHOD
MPI_Cart_sub(
    _In_ MPI_Comm comm,
    _In_ const int remain_dims[],
    _Out_ MPI_Comm* newcomm
    );

MPI_METHOD
PMPI_Cart_sub(
    _In_ MPI_Comm comm,
    _In_ const int remain_dims[],
    _Out_ MPI_Comm* newcomm
    );

MPI_METHOD
MPI_Cart_map(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int ndims,
    _In_reads_opt_(ndims) const int dims[],
    _In_reads_opt_(ndims) const int periods[],
    _Out_ _Deref_out_range_(>=, MPI_UNDEFINED) int* newrank
    );

MPI_METHOD
PMPI_Cart_map(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int ndims,
    _In_reads_opt_(ndims) const int dims[],
    _In_reads_opt_(ndims) const int periods[],
    _Out_ _Deref_out_range_(>=, MPI_UNDEFINED) int* newrank
    );

MPI_METHOD
MPI_Graph_map(
    _In_ MPI_Comm comm,
    _In_range_(>, 0) int nnodes,
    _In_reads_opt_(nnodes) const int index[],
    _In_opt_ const int edges[],
    _Out_ _Deref_out_range_(>=, MPI_UNDEFINED) int* newrank
    );

MPI_METHOD
PMPI_Graph_map(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int nnodes,
    _In_reads_opt_(nnodes) const int index[],
    _In_opt_ const int edges[],
    _Out_ _Deref_out_range_(>=, MPI_UNDEFINED) int* newrank
    );

MPI_METHOD
MPI_Dist_graph_neighbors_count(
    _In_ MPI_Comm comm,
    _Out_ _Deref_out_range_(>=, 0) int *indegree,
    _Out_ _Deref_out_range_(>=, 0) int *outdegree,
    _Out_ _Deref_out_range_(>=, 0) int *weighted
    );

MPI_METHOD
PMPI_Dist_graph_neighbors_count(
    _In_ MPI_Comm comm,
    _Out_ _Deref_out_range_(>=, 0) int *indegree,
    _Out_ _Deref_out_range_(>=, 0) int *outdegree,
    _Out_ _Deref_out_range_(>=, 0) int *weighted
    );

MPI_METHOD
MPI_Dist_graph_neighbors(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int maxindegree,
    _Out_writes_opt_(maxindegree) int sources[],
    _Out_writes_opt_(maxindegree) int sourceweights[],
    _In_range_(>=, 0) int maxoutdegree,
    _Out_writes_opt_(maxoutdegree) int destinations[],
    _Out_writes_opt_(maxoutdegree) int destweights[]
    );

MPI_METHOD
PMPI_Dist_graph_neighbors(
    _In_ MPI_Comm comm,
    _In_range_(>=, 0) int maxindegree,
    _Out_writes_opt_(maxindegree) int sources[],
    _Out_writes_opt_(maxindegree) int sourceweights[],
    _In_range_(>=, 0) int maxoutdegree,
    _Out_writes_opt_(maxoutdegree) int destinations[],
    _Out_writes_opt_(maxoutdegree) int destweights[]
    );

/*---------------------------------------------------------------------------*/
/* Chapter 8: Environmental Management                                       */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------*/
/* Section 8.1: Implementation Information     */
/*---------------------------------------------*/

#define MPI_VERSION     2
#define MPI_SUBVERSION  0

MPI_METHOD
MPI_Get_version(
    _Out_ int* version,
    _Out_ int* subversion
    );

MPI_METHOD
PMPI_Get_version(
    _Out_ int* version,
    _Out_ int* subversion
    );

#define MPI_MAX_LIBRARY_VERSION_STRING  64

MPI_METHOD
MPI_Get_library_version(
    _Out_writes_z_(MPI_MAX_LIBRARY_VERSION_STRING) char* version,
    _Out_ int* resultlen
);

MPI_METHOD
PMPI_Get_library_version(
    _Out_writes_z_(MPI_MAX_LIBRARY_VERSION_STRING) char* version,
    _Out_ int* resultlen
);

#define MPI_MAX_PROCESSOR_NAME  128

MPI_METHOD
MPI_Get_processor_name(
    _Out_writes_z_(MPI_MAX_PROCESSOR_NAME) char* name,
    _Out_ int* resultlen
    );

MPI_METHOD
PMPI_Get_processor_name(
    _Out_writes_z_(MPI_MAX_PROCESSOR_NAME) char* name,
    _Out_ int* resultlen
    );

/*---------------------------------------------*/
/* Section 8.2: Memory Allocation              */
/*---------------------------------------------*/

MPI_METHOD
MPI_Alloc_mem(
    _In_ MPI_Aint size,
    _In_ MPI_Info info,
    _Out_ void* baseptr
    );

MPI_METHOD
PMPI_Alloc_mem(
    _In_ MPI_Aint size,
    _In_ MPI_Info info,
    _Out_ void* baseptr
    );

MPI_METHOD
MPI_Free_mem(
    _In_ _Post_invalid_ void* base
    );

MPI_METHOD
PMPI_Free_mem(
    _In_ _Post_invalid_ void* base
    );


/*---------------------------------------------*/
/* Section 8.3: Error Handling                 */
/*---------------------------------------------*/

typedef
void
(MPIAPI MPI_Comm_errhandler_fn)(
    _In_ MPI_Comm* comm,
    _Inout_ int* errcode,
    ...
    );

MPI_METHOD
MPI_Comm_create_errhandler(
    _In_ MPI_Comm_errhandler_fn* function,
    _Out_ MPI_Errhandler* errhandler
    );

MPI_METHOD
PMPI_Comm_create_errhandler(
    _In_ MPI_Comm_errhandler_fn* function,
    _Out_ MPI_Errhandler* errhandler
    );

MPI_METHOD
MPI_Comm_set_errhandler(
    _In_ MPI_Comm comm,
    _In_ MPI_Errhandler errhandler
    );

MPI_METHOD
PMPI_Comm_set_errhandler(
    _In_ MPI_Comm comm,
    _In_ MPI_Errhandler errhandler
    );

MPI_METHOD
MPI_Comm_get_errhandler(
    _In_ MPI_Comm comm,
    _Out_ MPI_Errhandler* errhandler
    );

MPI_METHOD
PMPI_Comm_get_errhandler(
    _In_ MPI_Comm comm,
    _Out_ MPI_Errhandler* errhandler
    );


typedef
void
(MPIAPI MPI_Win_errhandler_fn)(
    _In_ MPI_Win* win,
    _Inout_ int* errcode,
    ...
    );

MPI_METHOD
MPI_Win_create_errhandler(
    _In_ MPI_Win_errhandler_fn* function,
    _Out_ MPI_Errhandler* errhandler
    );

MPI_METHOD
PMPI_Win_create_errhandler(
    _In_ MPI_Win_errhandler_fn* function,
    _Out_ MPI_Errhandler* errhandler
    );

MPI_METHOD
MPI_Win_set_errhandler(
    _In_ MPI_Win win,
    _In_ MPI_Errhandler errhandler
    );

MPI_METHOD
PMPI_Win_set_errhandler(
    _In_ MPI_Win win,
    _In_ MPI_Errhandler errhandler
    );

MPI_METHOD
MPI_Win_get_errhandler(
    _In_ MPI_Win win,
    _Out_ MPI_Errhandler* errhandler
    );

MPI_METHOD
PMPI_Win_get_errhandler(
    _In_ MPI_Win win,
    _Out_ MPI_Errhandler* errhandler
    );


typedef
void
(MPIAPI MPI_File_errhandler_fn)(
    _In_ MPI_File* file,
    _Inout_ int* errcode,
    ...
    );

MPI_METHOD
MPI_File_create_errhandler(
    _In_ MPI_File_errhandler_fn* function,
    _Out_ MPI_Errhandler* errhandler
    );

MPI_METHOD
PMPI_File_create_errhandler(
    _In_ MPI_File_errhandler_fn* function,
    _Out_ MPI_Errhandler* errhandler
    );

MPI_METHOD
MPI_File_set_errhandler(
    _In_ MPI_File file,
    _In_ MPI_Errhandler errhandler
    );

MPI_METHOD
PMPI_File_set_errhandler(
    _In_ MPI_File file,
    _In_ MPI_Errhandler errhandler
    );

MPI_METHOD
MPI_File_get_errhandler(
    _In_ MPI_File file,
    _Out_ MPI_Errhandler* errhandler
    );

MPI_METHOD
PMPI_File_get_errhandler(
    _In_ MPI_File file,
    _Out_ MPI_Errhandler* errhandler
    );

MPI_METHOD
MPI_Errhandler_free(
    _Inout_ MPI_Errhandler* errhandler
    );

MPI_METHOD
PMPI_Errhandler_free(
    _Inout_ MPI_Errhandler* errhandler
    );

#define MPI_MAX_ERROR_STRING    512

MPI_METHOD
MPI_Error_string(
    _In_ int errorcode,
    _Out_writes_z_(MPI_MAX_ERROR_STRING) char* string,
    _Out_ int* resultlen
    );

MPI_METHOD
PMPI_Error_string(
    _In_ int errorcode,
    _Out_writes_z_(MPI_MAX_ERROR_STRING) char* string,
    _Out_ int* resultlen
    );


/*---------------------------------------------*/
/* Section 8.4: Error Codes and Classes        */
/*---------------------------------------------*/

MPI_METHOD
MPI_Error_class(
    _In_ int errorcode,
    _Out_ int* errorclass
    );

MPI_METHOD
PMPI_Error_class(
    _In_ int errorcode,
    _Out_ int* errorclass
    );

MPI_METHOD
MPI_Add_error_class(
    _Out_ int* errorclass
    );

MPI_METHOD
PMPI_Add_error_class(
    _Out_ int* errorclass
    );

MPI_METHOD
MPI_Add_error_code(
    _In_ int errorclass,
    _Out_ int* errorcode
    );

MPI_METHOD
PMPI_Add_error_code(
    _In_ int errorclass,
    _Out_ int* errorcode
    );

MPI_METHOD
MPI_Add_error_string(
    _In_ int errorcode,
    _In_z_ const char* string
    );

MPI_METHOD
PMPI_Add_error_string(
    _In_ int errorcode,
    _In_z_ const char* string
    );

MPI_METHOD
MPI_Comm_call_errhandler(
    _In_ MPI_Comm comm,
    _In_ int errorcode
    );

MPI_METHOD
PMPI_Comm_call_errhandler(
    _In_ MPI_Comm comm,
    _In_ int errorcode
    );

MPI_METHOD
MPI_Win_call_errhandler(
    _In_ MPI_Win win,
    _In_ int errorcode
    );

MPI_METHOD
PMPI_Win_call_errhandler(
    _In_ MPI_Win win,
    _In_ int errorcode
    );

MPI_METHOD
MPI_File_call_errhandler(
    _In_ MPI_File file,
    _In_ int errorcode
    );

MPI_METHOD
PMPI_File_call_errhandler(
    _In_ MPI_File file,
    _In_ int errorcode
    );


/*---------------------------------------------*/
/* Section 8.6: Timers and Synchronization     */
/*---------------------------------------------*/

double
MPIAPI
MPI_Wtime(
    void
    );

double
MPIAPI
PMPI_Wtime(
    void
    );

double
MPIAPI
MPI_Wtick(
    void
    );

double
MPIAPI
PMPI_Wtick(
    void
    );


/*---------------------------------------------*/
/* Section 8.7: Startup                        */
/*---------------------------------------------*/

MPI_METHOD
MPI_Init(
    _In_opt_ const int* argc,
    _Notref_ _In_reads_opt_(*argc) char*** argv
    );

MPI_METHOD
PMPI_Init(
    _In_opt_ int* argc,
    _Notref_ _In_reads_opt_(*argc) char*** argv
    );

MPI_METHOD
MPI_Finalize(
    void
    );

MPI_METHOD
PMPI_Finalize(
    void
    );

MPI_METHOD
MPI_Initialized(
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
PMPI_Initialized(
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
MPI_Abort(
    _In_ MPI_Comm comm,
    _In_ int errorcode
    );

MPI_METHOD
PMPI_Abort(
    _In_ MPI_Comm comm,
    _In_ int errorcode
    );

MPI_METHOD
MPI_Finalized(
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
PMPI_Finalized(
    _mpi_out_flag_ int* flag
    );


/*---------------------------------------------------------------------------*/
/* Chapter 9: The Info Object                                                */
/*---------------------------------------------------------------------------*/

#define MPI_MAX_INFO_KEY    255
#define MPI_MAX_INFO_VAL   1024

MPI_METHOD
MPI_Info_create(
    _Out_ MPI_Info* info
    );

MPI_METHOD
PMPI_Info_create(
    _Out_ MPI_Info* info
    );

MPI_METHOD
MPI_Info_set(
    _In_ MPI_Info info,
    _In_z_ const char* key,
    _In_z_ const char* value
    );

MPI_METHOD
PMPI_Info_set(
    _In_ MPI_Info info,
    _In_z_ const char* key,
    _In_z_ const char* value
    );

MPI_METHOD
MPI_Info_delete(
    _In_ MPI_Info info,
    _In_z_ const char* key
    );

MPI_METHOD
PMPI_Info_delete(
    _In_ MPI_Info info,
    _In_z_ const char* key
    );

MPI_METHOD
MPI_Info_get(
    _In_ MPI_Info info,
    _In_z_ const char* key,
    _In_ int valuelen,
    _When_(*flag != 0, _Out_writes_z_(valuelen)) char* value,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
PMPI_Info_get(
    _In_ MPI_Info info,
    _In_z_ const char* key,
    _In_ int valuelen,
    _When_(*flag != 0, _Out_writes_z_(valuelen)) char* value,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
MPI_Info_get_valuelen(
    _In_ MPI_Info info,
    _In_z_ const char* key,
    _Out_ _Deref_out_range_(0, MPI_MAX_INFO_VAL) int* valuelen,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
PMPI_Info_get_valuelen(
    _In_ MPI_Info info,
    _In_z_ const char* key,
    _Out_ _Deref_out_range_(0, MPI_MAX_INFO_VAL) int* valuelen,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
MPI_Info_get_nkeys(
    _In_ MPI_Info info,
    _Out_ int* nkeys
    );

MPI_METHOD
PMPI_Info_get_nkeys(
    _In_ MPI_Info info,
    _Out_ int* nkeys
    );

MPI_METHOD
MPI_Info_get_nthkey(
    _In_ MPI_Info info,
    _In_range_(>=, 0) int n,
    _Out_writes_z_(MPI_MAX_INFO_KEY) char* key
    );

MPI_METHOD
PMPI_Info_get_nthkey(
    _In_ MPI_Info info,
    _In_range_(>=, 0) int n,
    _Out_writes_z_(MPI_MAX_INFO_KEY) char* key
    );

MPI_METHOD
MPI_Info_dup(
    _In_ MPI_Info info,
    _Out_ MPI_Info* newinfo
    );

MPI_METHOD
PMPI_Info_dup(
    _In_ MPI_Info info,
    _Out_ MPI_Info* newinfo
    );

MPI_METHOD
MPI_Info_free(
    _Inout_ MPI_Info* info
    );

MPI_METHOD
PMPI_Info_free(
    _Inout_ MPI_Info* info
    );


/*---------------------------------------------------------------------------*/
/* Chapter 10: Process Creation and Management                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------*/
/* Section 10.3: Process Manager Interface     */
/*---------------------------------------------*/

#define MPI_ARGV_NULL ((char**)0)
#define MPI_ARGVS_NULL ((char***)0)

#define MPI_ERRCODES_IGNORE ((int*)0)

MPI_METHOD
MPI_Comm_spawn(
    _In_z_ const char* command,
    _In_ char* argv[],
    _In_range_(>=, 0) int maxprocs,
    _In_ MPI_Info info,
    _In_range_(>=, 0) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Comm* intercomm,
    _Out_writes_(maxprocs) int array_of_errcodes[]
    );

MPI_METHOD
PMPI_Comm_spawn(
    _In_z_ const char* command,
    _In_ char* argv[],
    _In_range_(>=, 0) int maxprocs,
    _In_ MPI_Info info,
    _In_range_(>=, 0) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Comm* intercomm,
    _Out_writes_(maxprocs) int array_of_errcodes[]
    );

MPI_METHOD
MPI_Comm_get_parent(
    _Out_ MPI_Comm* parent
    );

MPI_METHOD
PMPI_Comm_get_parent(
    _Out_ MPI_Comm* parent
    );

MPI_METHOD
MPI_Comm_spawn_multiple(
    _In_range_(>, 0) int count,
    _In_reads_z_(count) char* array_of_commands[],
    _In_reads_z_(count) char** array_of_argv[],
    _In_reads_(count) const int array_of_maxprocs[],
    _In_reads_(count) const MPI_Info array_of_info[],
    _In_range_(>=, 0) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Comm* intercomm,
    _Out_ int array_of_errcodes[]
    );

MPI_METHOD
PMPI_Comm_spawn_multiple(
    _In_range_(>, 0) int count,
    _In_reads_z_(count) char* array_of_commands[],
    _In_reads_z_(count) char** array_of_argv[],
    _In_reads_(count) const int array_of_maxprocs[],
    _In_reads_(count) const MPI_Info array_of_info[],
    _In_range_(>=, 0) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Comm* intercomm,
    _Out_ int array_of_errcodes[]
    );


/*---------------------------------------------*/
/* Section 10.4: Establishing Communication    */
/*---------------------------------------------*/

#define MPI_MAX_PORT_NAME   256

MPI_METHOD
MPI_Open_port(
    _In_ MPI_Info info,
    _Out_writes_z_(MPI_MAX_PORT_NAME) char* port_name
    );

MPI_METHOD
PMPI_Open_port(
    _In_ MPI_Info info,
    _Out_writes_z_(MPI_MAX_PORT_NAME) char* port_name
    );

MPI_METHOD
MPI_Close_port(
    _In_z_ const char* port_name
    );

MPI_METHOD
PMPI_Close_port(
    _In_z_ const char* port_name
    );

MPI_METHOD
MPI_Comm_accept(
    _In_z_ const char* port_name,
    _In_ MPI_Info info,
    _In_range_(>=, 0) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Comm* newcomm
    );

MPI_METHOD
PMPI_Comm_accept(
    _In_z_ const char* port_name,
    _In_ MPI_Info info,
    _In_range_(>=, 0) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Comm* newcomm
    );

MPI_METHOD
MPI_Comm_connect(
    _In_z_ const char* port_name,
    _In_ MPI_Info info,
    _In_range_(>=, 0) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Comm* newcomm
    );

MPI_METHOD
PMPI_Comm_connect(
    _In_z_ const char* port_name,
    _In_ MPI_Info info,
    _In_range_(>=, 0) int root,
    _In_ MPI_Comm comm,
    _Out_ MPI_Comm* newcomm
    );


/*---------------------------------------------*/
/* Section 10.4.4: Name Publishing             */
/*---------------------------------------------*/

MPI_METHOD
MPI_Publish_name(
    _In_z_ const char* service_name,
    _In_ MPI_Info info,
    _In_z_ const char* port_name
    );

MPI_METHOD
PMPI_Publish_name(
    _In_z_ const char* service_name,
    _In_ MPI_Info info,
    _In_z_ const char* port_name
    );

MPI_METHOD
MPI_Unpublish_name(
    _In_z_ const char* service_name,
    _In_ MPI_Info info,
    _In_z_ const char* port_name
    );

MPI_METHOD
PMPI_Unpublish_name(
    _In_z_ const char* service_name,
    _In_ MPI_Info info,
    _In_z_ const char* port_name
    );

MPI_METHOD
MPI_Lookup_name(
    _In_z_ const char* service_name,
    _In_ MPI_Info info,
    _Out_writes_z_(MPI_MAX_PORT_NAME) char* port_name
    );

MPI_METHOD
PMPI_Lookup_name(
    _In_z_ const char* service_name,
    _In_ MPI_Info info,
    _Out_writes_z_(MPI_MAX_PORT_NAME) char* port_name
    );


/*---------------------------------------------*/
/* Section 10.5: Other Functionality           */
/*---------------------------------------------*/

MPI_METHOD
MPI_Comm_disconnect(
    _In_ MPI_Comm* comm
    );

MPI_METHOD
PMPI_Comm_disconnect(
    _In_ MPI_Comm* comm
    );

MPI_METHOD
MPI_Comm_join(
    _In_ int fd,
    _Out_ MPI_Comm* intercomm
    );

MPI_METHOD
PMPI_Comm_join(
    _In_ int fd,
    _Out_ MPI_Comm* intercomm
    );


/*---------------------------------------------------------------------------*/
/* Chapter 11: One-Sided Communications                                      */
/*---------------------------------------------------------------------------*/

MPI_METHOD
MPI_Win_create(
    _In_ void* base,
    _In_range_(>=, 0)  MPI_Aint size,
    _In_range_(>, 0)  int disp_unit,
    _In_ MPI_Info info,
    _In_ MPI_Comm comm,
    _Out_ MPI_Win* win
    );

MPI_METHOD
PMPI_Win_create(
    _In_ void* base,
    _In_range_(>=, 0)  MPI_Aint size,
    _In_range_(>, 0)  int disp_unit,
    _In_ MPI_Info info,
    _In_ MPI_Comm comm,
    _Out_ MPI_Win* win
    );

MPI_METHOD
MPI_Win_allocate(
    _In_range_(>= , 0) MPI_Aint size,
    _In_range_(>, 0) int disp_unit,
    _In_ MPI_Info info,
    _In_ MPI_Comm comm,
    _Out_ void *baseptr,
    _Out_ MPI_Win *win
    );

MPI_METHOD
PMPI_Win_allocate(
    _In_range_(>= , 0) MPI_Aint size,
    _In_range_(>, 0) int disp_unit,
    _In_ MPI_Info info,
    _In_ MPI_Comm comm,
    _Out_ void *baseptr,
    _Out_ MPI_Win *win
    );

MPI_METHOD
MPI_Win_allocate_shared(
    _In_range_(>=, 0) MPI_Aint size,
    _In_range_(>, 0) int disp_unit,
    _In_ MPI_Info info,
    _In_ MPI_Comm comm,
    _Out_ void *baseptr,
    _Out_ MPI_Win *win
    );

MPI_METHOD
PMPI_Win_allocate_shared(
    _In_range_(>=, 0) MPI_Aint size,
    _In_range_(>, 0) int disp_unit,
    _In_ MPI_Info info,
    _In_ MPI_Comm comm,
    _Out_ void *baseptr,
    _Out_ MPI_Win *win
    );

MPI_METHOD
MPI_Win_shared_query(
    _In_ MPI_Win win,
    _In_range_(>=, MPI_PROC_NULL) int rank,
    _Out_ MPI_Aint *size,
    _Out_ int *disp_unit,
    _Out_ void *baseptr
    );

MPI_METHOD
PMPI_Win_shared_query(
    _In_ MPI_Win win,
    _In_range_(>=, MPI_PROC_NULL) int rank,
    _Out_ MPI_Aint *size,
    _Out_ int *disp_unit,
    _Out_ void *baseptr
    );

MPI_METHOD
MPI_Win_create_dynamic(
    _In_ MPI_Info info,
    _In_ MPI_Comm comm,
    _Out_ MPI_Win* win
    );

MPI_METHOD
PMPI_Win_create_dynamic(
    _In_ MPI_Info info,
    _In_ MPI_Comm comm,
    _Out_ MPI_Win* win
    );

MPI_METHOD
MPI_Win_free(
    _Inout_ MPI_Win* win
    );

MPI_METHOD
PMPI_Win_free(
    _Inout_ MPI_Win* win
    );

MPI_METHOD
MPI_Win_get_group(
    _In_ MPI_Win win,
    _Out_ MPI_Group* group
    );

MPI_METHOD
PMPI_Win_get_group(
    _In_ MPI_Win win,
    _Out_ MPI_Group* group
    );

MPI_METHOD
MPI_Win_attach(
    _In_ MPI_Win win,
    _In_ void* base,
    _In_range_(>=, 0)  MPI_Aint size
    );

MPI_METHOD
PMPI_Win_attach(
    _In_ MPI_Win win,
    _In_ void* base,
    _In_range_(>=, 0)  MPI_Aint size
    );

MPI_METHOD
MPI_Win_detach(
    _In_ MPI_Win win,
    _In_ void* base
    );

MPI_METHOD
PMPI_Win_detach(
    _In_ MPI_Win win,
    _In_ void* base
    );

MPI_METHOD
MPI_Put(
    _In_opt_ const void* origin_addr,
    _In_range_(>=, 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>=, MPI_PROC_NULL) int target_rank,
    _In_range_(>=, 0) MPI_Aint target_disp,
    _In_range_(>=, 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Put(
    _In_opt_ const void* origin_addr,
    _In_range_(>=, 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>=, MPI_PROC_NULL) int target_rank,
    _In_range_(>=, 0) MPI_Aint target_disp,
    _In_range_(>=, 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Rput(
    _In_opt_ const void* origin_addr,
    _In_range_(>=, 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>=, MPI_PROC_NULL) int target_rank,
    _In_range_(>=, 0) MPI_Aint target_disp,
    _In_range_(>=, 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Win win,
    _Out_ MPI_Request *request
    );

MPI_METHOD
PMPI_Rput(
    _In_opt_ const void* origin_addr,
    _In_range_(>=, 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>=, MPI_PROC_NULL) int target_rank,
    _In_range_(>=, 0) MPI_Aint target_disp,
    _In_range_(>=, 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Win win,
    _Out_ MPI_Request *request
    );

MPI_METHOD
MPI_Get(
    _In_opt_ void* origin_addr,
    _In_range_(>=, 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>=, MPI_PROC_NULL) int target_rank,
    _In_range_(>=, 0) MPI_Aint target_disp,
    _In_range_(>=, 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Get(
    _In_opt_ void* origin_addr,
    _In_range_(>=, 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>=, MPI_PROC_NULL) int target_rank,
    _In_range_(>=, 0) MPI_Aint target_disp,
    _In_range_(>=, 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Rget(
    _In_opt_ void* origin_addr,
    _In_range_(>= , 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>= , MPI_PROC_NULL) int target_rank,
    _In_range_(>= , 0) MPI_Aint target_disp,
    _In_range_(>= , 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Win win,
    _Out_ MPI_Request *request
    );

MPI_METHOD
PMPI_Rget(
    _In_opt_ void* origin_addr,
    _In_range_(>= , 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>= , MPI_PROC_NULL) int target_rank,
    _In_range_(>= , 0) MPI_Aint target_disp,
    _In_range_(>= , 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Win win,
    _Out_ MPI_Request *request
    );

MPI_METHOD
MPI_Accumulate(
    _In_opt_ const void* origin_addr,
    _In_range_(>=, 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>=, MPI_PROC_NULL) int target_rank,
    _In_range_(>=, 0) MPI_Aint target_disp,
    _In_range_(>=, 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Op op,
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Accumulate(
    _In_opt_ const void* origin_addr,
    _In_range_(>=, 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>=, MPI_PROC_NULL) int target_rank,
    _In_range_(>=, 0) MPI_Aint target_disp,
    _In_range_(>=, 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Op op,
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Raccumulate(
    _In_opt_ const void* origin_addr,
    _In_range_(>=, 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>=, MPI_PROC_NULL) int target_rank,
    _In_range_(>=, 0) MPI_Aint target_disp,
    _In_range_(>=, 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Op op,
    _In_ MPI_Win win,
    _Out_ MPI_Request *request
    );

MPI_METHOD
PMPI_Raccumulate(
    _In_opt_ const void* origin_addr,
    _In_range_(>=, 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_range_(>=, MPI_PROC_NULL) int target_rank,
    _In_range_(>=, 0) MPI_Aint target_disp,
    _In_range_(>=, 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Op op,
    _In_ MPI_Win win,
    _Out_ MPI_Request *request
    );

MPI_METHOD
MPI_Get_accumulate(
    _In_opt_ const void* origin_addr,
    _In_range_(>= , 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_opt_ void* result_addr,
    _In_range_(>= , 0) int result_count,
    _In_ MPI_Datatype result_datatype,
    _In_range_(>= , MPI_PROC_NULL) int target_rank,
    _In_range_(>= , 0) MPI_Aint target_disp,
    _In_range_(>= , 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Op op,
    _In_ MPI_Win win
);

MPI_METHOD
PMPI_Get_accumulate(
    _In_opt_ const void* origin_addr,
    _In_range_(>= , 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_opt_ void* result_addr,
    _In_range_(>= , 0) int result_count,
    _In_ MPI_Datatype result_datatype,
    _In_range_(>= , MPI_PROC_NULL) int target_rank,
    _In_range_(>= , 0) MPI_Aint target_disp,
    _In_range_(>= , 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Op op,
    _In_ MPI_Win win
);

MPI_METHOD
MPI_Rget_accumulate(
    _In_opt_ const void* origin_addr,
    _In_range_(>= , 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_opt_ void* result_addr,
    _In_range_(>= , 0) int result_count,
    _In_ MPI_Datatype result_datatype,
    _In_range_(>= , MPI_PROC_NULL) int target_rank,
    _In_range_(>= , 0) MPI_Aint target_disp,
    _In_range_(>= , 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Op op,
    _In_ MPI_Win win,
    _Out_ MPI_Request *request
);

MPI_METHOD
PMPI_Rget_accumulate(
    _In_opt_ const void* origin_addr,
    _In_range_(>= , 0) int origin_count,
    _In_ MPI_Datatype origin_datatype,
    _In_opt_ void* result_addr,
    _In_range_(>= , 0) int result_count,
    _In_ MPI_Datatype result_datatype,
    _In_range_(>= , MPI_PROC_NULL) int target_rank,
    _In_range_(>= , 0) MPI_Aint target_disp,
    _In_range_(>= , 0) int target_count,
    _In_ MPI_Datatype target_datatype,
    _In_ MPI_Op op,
    _In_ MPI_Win win,
    _Out_ MPI_Request *request
);

MPI_METHOD
MPI_Fetch_and_op(
    _In_opt_ const void* origin_addr,
    _When_(target_rank != MPI_PROC_NULL, _In_)
    _When_(target_rank == MPI_PROC_NULL, _In_opt_)
        void* result_addr,
    _In_ MPI_Datatype datatype,
    _In_range_(>= , MPI_PROC_NULL) int target_rank,
    _In_range_(>= , 0) MPI_Aint target_disp,
    _In_ MPI_Op op,
    _In_ MPI_Win win
);

MPI_METHOD
PMPI_Fetch_and_op(
    _In_opt_ const void* origin_addr,
    _When_(target_rank != MPI_PROC_NULL, _In_)
    _When_(target_rank == MPI_PROC_NULL, _In_opt_)
        void* result_addr,
    _In_ MPI_Datatype datatype,
    _In_range_(>= , MPI_PROC_NULL) int target_rank,
    _In_range_(>= , 0) MPI_Aint target_disp,
    _In_ MPI_Op op,
    _In_ MPI_Win win
);

MPI_METHOD
MPI_Compare_and_swap(
    _When_(target_rank != MPI_PROC_NULL, _In_)
    _When_(target_rank == MPI_PROC_NULL, _In_opt_)
        const void* origin_addr,
    _When_(target_rank != MPI_PROC_NULL, _In_)
    _When_(target_rank == MPI_PROC_NULL, _In_opt_)
        const void* compare_addr,
    _When_(target_rank != MPI_PROC_NULL, _In_)
    _When_(target_rank == MPI_PROC_NULL, _In_opt_)
        void* result_addr,
    _In_ MPI_Datatype datatype,
    _In_range_(>= , MPI_PROC_NULL) int target_rank,
    _In_range_(>= , 0) MPI_Aint target_disp,
    _In_ MPI_Win win
);

MPI_METHOD
PMPI_Compare_and_swap(
    _When_(target_rank != MPI_PROC_NULL, _In_)
    _When_(target_rank == MPI_PROC_NULL, _In_opt_)
        const void* origin_addr,
    _When_(target_rank != MPI_PROC_NULL, _In_)
    _When_(target_rank == MPI_PROC_NULL, _In_opt_)
        const void* compare_addr,
    _When_(target_rank != MPI_PROC_NULL, _In_)
    _When_(target_rank == MPI_PROC_NULL, _In_opt_)
        void* result_addr,
    _In_ MPI_Datatype datatype,
    _In_range_(>= , MPI_PROC_NULL) int target_rank,
    _In_range_(>= , 0) MPI_Aint target_disp,
    _In_ MPI_Win win
);

/* Asserts for one-sided communication */
#define MPI_MODE_NOCHECK    1024
#define MPI_MODE_NOSTORE    2048
#define MPI_MODE_NOPUT      4096
#define MPI_MODE_NOPRECEDE  8192
#define MPI_MODE_NOSUCCEED 16384

MPI_METHOD
MPI_Win_fence(
    _In_ int assert,
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_fence(
    _In_ int assert,
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_start(
    _In_ MPI_Group group,
    _In_ int assert,
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_start(
    _In_ MPI_Group group,
    _In_ int assert,
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_complete(
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_complete(
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_post(
    _In_ MPI_Group group,
    _In_ int assert,
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_post(
    _In_ MPI_Group group,
    _In_ int assert,
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_wait(
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_wait(
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_test(
    _In_ MPI_Win win,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
PMPI_Win_test(
    _In_ MPI_Win win,
    _mpi_out_flag_ int* flag
    );

#define MPI_LOCK_EXCLUSIVE  234
#define MPI_LOCK_SHARED     235

MPI_METHOD
MPI_Win_lock(
    _In_ int lock_type,
    _In_range_(>=, MPI_PROC_NULL) int rank,
    _In_ int assert,
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_lock(
    _In_ int lock_type,
    _In_range_(>=, MPI_PROC_NULL) int rank,
    _In_ int assert,
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_lock_all(
    _In_ int assert,
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_lock_all(
    _In_ int assert,
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_unlock(
    _In_range_(>=, MPI_PROC_NULL) int rank,
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_unlock(
    _In_range_(>=, MPI_PROC_NULL) int rank,
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_unlock_all(
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_unlock_all(
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_flush(
    _In_range_(>=, MPI_PROC_NULL) int rank,
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_flush(
    _In_range_(>=, MPI_PROC_NULL) int rank,
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_flush_all(
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_flush_all(
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_flush_local(
    _In_range_(>= , MPI_PROC_NULL) int rank,
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_flush_local(
    _In_range_(>= , MPI_PROC_NULL) int rank,
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_flush_local_all(
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_flush_local_all(
    _In_ MPI_Win win
    );

MPI_METHOD
MPI_Win_sync(
    _In_ MPI_Win win
    );

MPI_METHOD
PMPI_Win_sync(
    _In_ MPI_Win win
    );

/*---------------------------------------------------------------------------*/
/* Chapter 12: External Interfaces                                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------*/
/* Section 12.2: Generalized Requests          */
/*---------------------------------------------*/

typedef
int
(MPIAPI MPI_Grequest_query_function)(
    _In_opt_ void* extra_state,
    _Out_ MPI_Status* status
    );

typedef
int
(MPIAPI MPI_Grequest_free_function)(
    _In_opt_ void* extra_state
    );

typedef
int
(MPIAPI MPI_Grequest_cancel_function)(
    _In_opt_ void* extra_state,
    _In_ int complete
    );

MPI_METHOD
MPI_Grequest_start(
    _In_ MPI_Grequest_query_function* query_fn,
    _In_ MPI_Grequest_free_function* free_fn,
    _In_ MPI_Grequest_cancel_function* cancel_fn,
    _In_opt_ void* extra_state,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_Grequest_start(
    _In_ MPI_Grequest_query_function* query_fn,
    _In_ MPI_Grequest_free_function* free_fn,
    _In_ MPI_Grequest_cancel_function* cancel_fn,
    _In_opt_ void* extra_state,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_Grequest_complete(
    _In_ MPI_Request request
    );

MPI_METHOD
PMPI_Grequest_complete(
    _In_ MPI_Request request
    );


/*---------------------------------------------*/
/* Section 12.3: Information with Status       */
/*---------------------------------------------*/

MPI_METHOD
MPI_Status_set_elements(
    _In_ MPI_Status* status,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, 0) int count
    );

MPI_METHOD
PMPI_Status_set_elements(
    _In_ MPI_Status* status,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, 0) int count
    );

MPI_METHOD
MPI_Status_set_elements_x(
    _In_ MPI_Status* status,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, 0) MPI_Count count
    );

MPI_METHOD
PMPI_Status_set_elements_x(
    _In_ MPI_Status* status,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, 0) MPI_Count count
    );

MPI_METHOD
MPI_Status_set_cancelled(
    _In_ MPI_Status* status,
    _In_range_(0,1) int flag
    );

MPI_METHOD
PMPI_Status_set_cancelled(
    _In_ MPI_Status* status,
    _In_range_(0,1) int flag
    );


/*---------------------------------------------*/
/* Section 12.4: Threads                       */
/*---------------------------------------------*/

#define MPI_THREAD_SINGLE       0
#define MPI_THREAD_FUNNELED     1
#define MPI_THREAD_SERIALIZED   2
#define MPI_THREAD_MULTIPLE     3

MPI_METHOD
MPI_Init_thread(
    _In_opt_ const int* argc,
    _Notref_ _In_reads_opt_(*argc) char*** argv,
    _In_ int required,
    _Out_ int* provided
    );

MPI_METHOD
PMPI_Init_thread(
    _In_opt_ int* argc,
    _Notref_ _In_reads_opt_(*argc) char*** argv,
    _In_ int required,
    _Out_ int* provided
    );

MPI_METHOD
MPI_Query_thread(
    _Out_ int* provided
    );

MPI_METHOD
PMPI_Query_thread(
    _Out_ int* provided
    );

MPI_METHOD
MPI_Is_thread_main(
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
PMPI_Is_thread_main(
    _mpi_out_flag_ int* flag
    );


/*---------------------------------------------------------------------------*/
/* Chapter 13: I/O                                                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------*/
/* Section 13.2: File Manipulation             */
/*---------------------------------------------*/

#define MPI_MODE_CREATE             0x00000001
#define MPI_MODE_RDONLY             0x00000002
#define MPI_MODE_WRONLY             0x00000004
#define MPI_MODE_RDWR               0x00000008
#define MPI_MODE_DELETE_ON_CLOSE    0x00000010
#define MPI_MODE_UNIQUE_OPEN        0x00000020
#define MPI_MODE_EXCL               0x00000040
#define MPI_MODE_APPEND             0x00000080
#define MPI_MODE_SEQUENTIAL         0x00000100
#define MSMPI_MODE_HIDDEN           0x00000200

MPI_METHOD
MPI_File_open(
    _In_ MPI_Comm comm,
    _In_z_ const char* filename,
    _In_ int amode,
    _In_ MPI_Info info,
    _Out_ MPI_File* fh
    );

MPI_METHOD
PMPI_File_open(
    _In_ MPI_Comm comm,
    _In_z_ const char* filename,
    _In_ int amode,
    _In_ MPI_Info info,
    _Out_ MPI_File* fh
    );

MPI_METHOD
MPI_File_close(
    _In_ MPI_File* fh
    );

MPI_METHOD
PMPI_File_close(
    _In_ MPI_File* fh
    );

MPI_METHOD
MPI_File_delete(
    _In_z_ const char* filename,
    _In_ MPI_Info info
    );

MPI_METHOD
PMPI_File_delete(
    _In_z_ const char* filename,
    _In_ MPI_Info info
    );

MPI_METHOD
MPI_File_set_size(
    _In_ MPI_File fh,
    _In_ MPI_Offset size
    );

MPI_METHOD
PMPI_File_set_size(
    _In_ MPI_File fh,
    _In_ MPI_Offset size
    );

MPI_METHOD
MPI_File_preallocate(
    _In_ MPI_File fh,
    _In_ MPI_Offset size
    );

MPI_METHOD
PMPI_File_preallocate(
    _In_ MPI_File fh,
    _In_ MPI_Offset size
    );

MPI_METHOD
MPI_File_get_size(
    _In_ MPI_File fh,
    _Out_ MPI_Offset* size
    );

MPI_METHOD
PMPI_File_get_size(
    _In_ MPI_File fh,
    _Out_ MPI_Offset* size
    );

MPI_METHOD
MPI_File_get_group(
    _In_ MPI_File fh,
    _Out_ MPI_Group* group
    );

MPI_METHOD
PMPI_File_get_group(
    _In_ MPI_File fh,
    _Out_ MPI_Group* group
    );

MPI_METHOD
MPI_File_get_amode(
    _In_ MPI_File fh,
    _Out_ int* amode
    );

MPI_METHOD
PMPI_File_get_amode(
    _In_ MPI_File fh,
    _Out_ int* amode
    );

MPI_METHOD
MPI_File_set_info(
    _In_ MPI_File fh,
    _In_ MPI_Info info
    );

MPI_METHOD
PMPI_File_set_info(
    _In_ MPI_File fh,
    _In_ MPI_Info info
    );

MPI_METHOD
MPI_File_get_info(
    _In_ MPI_File fh,
    _Out_ MPI_Info* info_used
    );

MPI_METHOD
PMPI_File_get_info(
    _In_ MPI_File fh,
    _Out_ MPI_Info* info_used
    );


/*---------------------------------------------*/
/* Section 13.3: File Views                    */
/*---------------------------------------------*/

#define MPI_DISPLACEMENT_CURRENT (-54278278)

MPI_METHOD
MPI_File_set_view(
    _In_ MPI_File fh,
    _In_ MPI_Offset disp,
    _In_ MPI_Datatype etype,
    _In_ MPI_Datatype filetype,
    _In_z_ const char* datarep,
    _In_ MPI_Info info
    );

MPI_METHOD
PMPI_File_set_view(
    _In_ MPI_File fh,
    _In_ MPI_Offset disp,
    _In_ MPI_Datatype etype,
    _In_ MPI_Datatype filetype,
    _In_z_ const char* datarep,
    _In_ MPI_Info info
    );

#define MPI_MAX_DATAREP_STRING  128

MPI_METHOD
MPI_File_get_view(
    _In_ MPI_File fh,
    _Out_ MPI_Offset* disp,
    _Out_ MPI_Datatype* etype,
    _Out_ MPI_Datatype* filetype,
    _Out_writes_z_(MPI_MAX_DATAREP_STRING) char* datarep
    );

MPI_METHOD
PMPI_File_get_view(
    _In_ MPI_File fh,
    _Out_ MPI_Offset* disp,
    _Out_ MPI_Datatype* etype,
    _Out_ MPI_Datatype* filetype,
    _Out_writes_z_(MPI_MAX_DATAREP_STRING) char* datarep
    );


/*---------------------------------------------*/
/* Section 13.4: Data Access                   */
/*---------------------------------------------*/

MPI_METHOD
MPI_File_read_at(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_read_at(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_read_at_all(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_read_at_all(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_write_at(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_write_at(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_write_at_all(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_write_at_all(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_iread_at(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_File_iread_at(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_File_iwrite_at(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_File_iwrite_at(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_File_read(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_read(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_read_all(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_read_all(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_write(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_write(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_write_all(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_write_all(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );


MPI_METHOD
MPI_File_iread(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_File_iread(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_File_iwrite(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_File_iwrite(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );


/* File seek whence */
#define MPI_SEEK_SET    600
#define MPI_SEEK_CUR    602
#define MPI_SEEK_END    604

MPI_METHOD
MPI_File_seek(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_ int whence
    );

MPI_METHOD
PMPI_File_seek(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_ int whence
    );

MPI_METHOD
MPI_File_get_position(
    _In_ MPI_File fh,
    _Out_ MPI_Offset* offset
    );

MPI_METHOD
PMPI_File_get_position(
    _In_ MPI_File fh,
    _Out_ MPI_Offset* offset
    );

MPI_METHOD
MPI_File_get_byte_offset(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _Out_ MPI_Offset* disp
    );

MPI_METHOD
PMPI_File_get_byte_offset(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _Out_ MPI_Offset* disp
    );

MPI_METHOD
MPI_File_read_shared(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
     );

MPI_METHOD
PMPI_File_read_shared(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
     );

MPI_METHOD
MPI_File_write_shared(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_write_shared(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_iread_shared(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_File_iread_shared(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_File_iwrite_shared(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );

MPI_METHOD
PMPI_File_iwrite_shared(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Request* request
    );

MPI_METHOD
MPI_File_read_ordered(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_read_ordered(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_write_ordered(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_write_ordered(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_seek_shared(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_ int whence
    );

MPI_METHOD
PMPI_File_seek_shared(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_ int whence
    );

MPI_METHOD
MPI_File_get_position_shared(
    _In_ MPI_File fh,
    _Out_ MPI_Offset* offset
    );

MPI_METHOD
PMPI_File_get_position_shared(
    _In_ MPI_File fh,
    _Out_ MPI_Offset* offset
    );

MPI_METHOD
MPI_File_read_at_all_begin(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
PMPI_File_read_at_all_begin(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
MPI_File_read_at_all_end(
    _In_ MPI_File fh,
    _Out_ void* buf,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_read_at_all_end(
    _In_ MPI_File fh,
    _Out_ void* buf,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_write_at_all_begin(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
PMPI_File_write_at_all_begin(
    _In_ MPI_File fh,
    _In_ MPI_Offset offset,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
MPI_File_write_at_all_end(
    _In_ MPI_File fh,
    _In_ const void* buf,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_write_at_all_end(
    _In_ MPI_File fh,
    _In_ const void* buf,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_read_all_begin(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
PMPI_File_read_all_begin(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
MPI_File_read_all_end(
    _In_ MPI_File fh,
    _Out_ void* buf,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_read_all_end(
    _In_ MPI_File fh,
    _Out_ void* buf,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_write_all_begin(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
PMPI_File_write_all_begin(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
MPI_File_write_all_end(
    _In_ MPI_File fh,
    _In_ const void* buf,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_write_all_end(
    _In_ MPI_File fh,
    _In_ const void* buf,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_read_ordered_begin(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
PMPI_File_read_ordered_begin(
    _In_ MPI_File fh,
    _Out_opt_ void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
MPI_File_read_ordered_end(
    _In_ MPI_File fh,
    _Out_ void* buf,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_read_ordered_end(
    _In_ MPI_File fh,
    _Out_ void* buf,
    _Out_ MPI_Status* status
    );

MPI_METHOD
MPI_File_write_ordered_begin(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
PMPI_File_write_ordered_begin(
    _In_ MPI_File fh,
    _In_opt_ const void* buf,
    _In_range_(>=, 0) int count,
    _In_ MPI_Datatype datatype
    );

MPI_METHOD
MPI_File_write_ordered_end(
    _In_ MPI_File fh,
    _In_ const void* buf,
    _Out_ MPI_Status* status
    );

MPI_METHOD
PMPI_File_write_ordered_end(
    _In_ MPI_File fh,
    _In_ const void* buf,
    _Out_ MPI_Status* status
    );


/*---------------------------------------------*/
/* Section 13.5: File Interoperability         */
/*---------------------------------------------*/

MPI_METHOD
MPI_File_get_type_extent(
    _In_ MPI_File fh,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Aint* extent
    );

MPI_METHOD
PMPI_File_get_type_extent(
    _In_ MPI_File fh,
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Aint* extent
    );


typedef
int
(MPIAPI MPI_Datarep_conversion_function)(
    _Inout_ void* userbuf,
    _In_ MPI_Datatype datatype,
    _In_range_(>=, 0) int count,
    _Inout_ void* filebuf,
    _In_ MPI_Offset position,
    _In_opt_ void* extra_state
    );

typedef
int
(MPIAPI MPI_Datarep_extent_function)(
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Aint* file_extent,
    _In_opt_ void* extra_state
    );

#define MPI_CONVERSION_FN_NULL ((MPI_Datarep_conversion_function*)0)

MPI_METHOD
MPI_Register_datarep(
    _In_z_ const char* datarep,
    _In_opt_ MPI_Datarep_conversion_function* read_conversion_fn,
    _In_opt_ MPI_Datarep_conversion_function* write_conversion_fn,
    _In_ MPI_Datarep_extent_function* dtype_file_extent_fn,
    _In_opt_ void* extra_state
    );

MPI_METHOD
PMPI_Register_datarep(
    _In_z_ const char* datarep,
    _In_opt_ MPI_Datarep_conversion_function* read_conversion_fn,
    _In_opt_ MPI_Datarep_conversion_function* write_conversion_fn,
    _In_ MPI_Datarep_extent_function* dtype_file_extent_fn,
    _In_opt_ void* extra_state
    );


/*---------------------------------------------*/
/* Section 13.6: Consistency and Semantics     */
/*---------------------------------------------*/

MPI_METHOD
MPI_File_set_atomicity(
    _In_ MPI_File fh,
    _In_range_(0, 1) int flag
    );

MPI_METHOD
PMPI_File_set_atomicity(
    _In_ MPI_File fh,
    _In_range_(0, 1) int flag
    );

MPI_METHOD
MPI_File_get_atomicity(
    _In_ MPI_File fh,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
PMPI_File_get_atomicity(
    _In_ MPI_File fh,
    _mpi_out_flag_ int* flag
    );

MPI_METHOD
MPI_File_sync(
    _In_ MPI_File fh
    );

MPI_METHOD
PMPI_File_sync(
    _In_ MPI_File fh
    );


/*---------------------------------------------------------------------------*/
/* Chapter 14: Profiling Interface                                           */
/*---------------------------------------------------------------------------*/

MPI_METHOD
MPI_Pcontrol(
    _In_ const int level,
    ...
    );

MPI_METHOD
PMPI_Pcontrol(
    _In_ const int level,
    ...
    );


/*---------------------------------------------------------------------------*/
/* Chapter 15: Deprecated Functions                                          */
/*---------------------------------------------------------------------------*/

#ifdef MSMPI_NO_DEPRECATE_20
#define MSMPI_DEPRECATE_20( x )
#else
#define MSMPI_DEPRECATE_20( x ) __declspec(deprecated( \
    "Deprecated in MPI 2.0, use '" #x "'.  " \
    "To disable deprecation, define MSMPI_NO_DEPRECATE_20." ))
#endif

MSMPI_DEPRECATE_20( MPI_Type_create_hvector )
MPI_METHOD
MPI_Type_hvector(
    _In_range_(>=, 0) int count,
    _In_range_(>=, 0) int blocklength,
    _In_ MPI_Aint stride,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MSMPI_DEPRECATE_20( PMPI_Type_create_hvector )
MPI_METHOD
PMPI_Type_hvector(
    _In_range_(>=, 0) int count,
    _In_range_(>=, 0) int blocklength,
    _In_ MPI_Aint stride,
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MSMPI_DEPRECATE_20( MPI_Type_create_hindexed )
MPI_METHOD
MPI_Type_hindexed(
    _In_range_(>=, 0) int count,
    _In_reads_opt_(count) const int array_of_blocklengths[],
    _In_reads_opt_(count) const MPI_Aint array_of_displacements[],
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MSMPI_DEPRECATE_20( PMPI_Type_create_hindexed )
MPI_METHOD
PMPI_Type_hindexed(
    _In_range_(>=, 0) int count,
    _In_reads_opt_(count) const int array_of_blocklengths[],
    _In_reads_opt_(count) const MPI_Aint array_of_displacements[],
    _In_ MPI_Datatype oldtype,
    _Out_ MPI_Datatype* newtype
    );

MSMPI_DEPRECATE_20( MPI_Type_create_struct )
MPI_METHOD
MPI_Type_struct(
    _In_range_(>=, 0) int count,
    _In_reads_opt_(count) const int array_of_blocklengths[],
    _In_reads_opt_(count) const MPI_Aint array_of_displacements[],
    _In_reads_opt_(count) const MPI_Datatype array_of_types[],
    _Out_ MPI_Datatype* newtype
    );

MSMPI_DEPRECATE_20( PMPI_Type_create_struct )
MPI_METHOD
PMPI_Type_struct(
    _In_range_(>=, 0) int count,
    _In_reads_opt_(count) const int array_of_blocklengths[],
    _In_reads_opt_(count) const MPI_Aint array_of_displacements[],
    _In_reads_opt_(count) const MPI_Datatype array_of_types[],
    _Out_ MPI_Datatype* newtype
    );

MSMPI_DEPRECATE_20( MPI_Get_address )
MPI_METHOD
MPI_Address(
    _In_ void* location,
    _Out_ MPI_Aint* address
    );

MSMPI_DEPRECATE_20( PMPI_Get_address )
MPI_METHOD
PMPI_Address(
    _In_ void* location,
    _Out_ MPI_Aint* address
    );

MSMPI_DEPRECATE_20( MPI_Type_get_extent )
MPI_METHOD
MPI_Type_extent(
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Aint* extent
    );

MSMPI_DEPRECATE_20( PMPI_Type_get_extent )
MPI_METHOD
PMPI_Type_extent(
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Aint* extent
    );

MSMPI_DEPRECATE_20( MPI_Type_get_extent )
MPI_METHOD
MPI_Type_lb(
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Aint* displacement
    );

MSMPI_DEPRECATE_20( PMPI_Type_get_extent )
MPI_METHOD
PMPI_Type_lb(
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Aint* displacement
    );

MSMPI_DEPRECATE_20( MPI_Type_get_extent )
MPI_METHOD
MPI_Type_ub(
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Aint* displacement
    );

MSMPI_DEPRECATE_20( PMPI_Type_get_extent )
MPI_METHOD
PMPI_Type_ub(
    _In_ MPI_Datatype datatype,
    _Out_ MPI_Aint* displacement
    );


typedef MPI_Comm_copy_attr_function MPI_Copy_function;
typedef MPI_Comm_delete_attr_function MPI_Delete_function;

#define MPI_NULL_COPY_FN ((MPI_Copy_function*)0)
#define MPI_NULL_DELETE_FN ((MPI_Delete_function*)0)
#define MPI_DUP_FN MPIR_Dup_fn


MSMPI_DEPRECATE_20( MPI_Comm_create_keyval )
MPI_METHOD
MPI_Keyval_create(
    _In_opt_ MPI_Copy_function* copy_fn,
    _In_opt_ MPI_Delete_function* delete_fn,
    _Out_ int* keyval,
    _In_opt_ void* extra_state
    );

MSMPI_DEPRECATE_20( PMPI_Comm_create_keyval )
MPI_METHOD
PMPI_Keyval_create(
    _In_opt_ MPI_Copy_function* copy_fn,
    _In_opt_ MPI_Delete_function* delete_fn,
    _Out_ int* keyval,
    _In_opt_ void* extra_state
    );

MSMPI_DEPRECATE_20( MPI_Comm_free_keyval )
MPI_METHOD
MPI_Keyval_free(
    _Inout_ int* keyval
    );

MSMPI_DEPRECATE_20( PMPI_Comm_free_keyval )
MPI_METHOD
PMPI_Keyval_free(
    _Inout_ int* keyval
    );

MSMPI_DEPRECATE_20( MPI_Comm_set_attr )
MPI_METHOD
MPI_Attr_put(
    _In_ MPI_Comm comm,
    _In_ int keyval,
    _In_opt_ void* attribute_val
    );

MSMPI_DEPRECATE_20( PMPI_Comm_set_attr )
MPI_METHOD
PMPI_Attr_put(
    _In_ MPI_Comm comm,
    _In_ int keyval,
    _In_opt_ void* attribute_val
    );

MSMPI_DEPRECATE_20( MPI_Comm_get_attr )
MPI_METHOD
MPI_Attr_get(
    _In_ MPI_Comm comm,
    _In_ int keyval,
    _Out_ void* attribute_val,
    _mpi_out_flag_ int* flag
    );

MSMPI_DEPRECATE_20( PMPI_Comm_get_attr )
MPI_METHOD
PMPI_Attr_get(
    _In_ MPI_Comm comm,
    _In_ int keyval,
    _Out_ void* attribute_val,
    _mpi_out_flag_ int* flag
    );

MSMPI_DEPRECATE_20( MPI_Comm_delete_attr )
MPI_METHOD
MPI_Attr_delete(
    _In_ MPI_Comm comm,
    _In_ int keyval
    );

MSMPI_DEPRECATE_20( PMPI_Comm_delete_attr )
MPI_METHOD
PMPI_Attr_delete(
    _In_ MPI_Comm comm,
    _In_ int keyval
    );


typedef MPI_Comm_errhandler_fn MPI_Handler_function;

MSMPI_DEPRECATE_20( MPI_Comm_create_errhandler )
MPI_METHOD
MPI_Errhandler_create(
    _In_ MPI_Handler_function* function,
    _Out_ MPI_Errhandler* errhandler
    );

MSMPI_DEPRECATE_20( PMPI_Comm_create_errhandler )
MPI_METHOD
PMPI_Errhandler_create(
    _In_ MPI_Handler_function* function,
    _Out_ MPI_Errhandler* errhandler
    );

MSMPI_DEPRECATE_20( MPI_Comm_set_errhandler )
MPI_METHOD
MPI_Errhandler_set(
    _In_ MPI_Comm comm,
    _In_ MPI_Errhandler errhandler
    );

MSMPI_DEPRECATE_20( PMPI_Comm_set_errhandler )
MPI_METHOD
PMPI_Errhandler_set(
    _In_ MPI_Comm comm,
    _In_ MPI_Errhandler errhandler
    );

MSMPI_DEPRECATE_20( MPI_Comm_get_errhandler )
MPI_METHOD
MPI_Errhandler_get(
    _In_ MPI_Comm comm,
    _Out_ MPI_Errhandler* errhandler
    );

MSMPI_DEPRECATE_20( PMPI_Comm_get_errhandler )
MPI_METHOD
PMPI_Errhandler_get(
    _In_ MPI_Comm comm,
    _Out_ MPI_Errhandler* errhandler
    );


/*---------------------------------------------------------------------------*/
/* Chapter 16: Language Bindings                                             */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------*/
/* Section 16.2: Fortran Support               */
/*---------------------------------------------*/

MPI_METHOD
MPI_Type_create_f90_real(
    _In_ int p,
    _In_ int r,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_create_f90_real(
    _In_ int p,
    _In_ int r,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
MPI_Type_create_f90_complex(
    _In_ int p,
    _In_ int r,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_create_f90_complex(
    _In_ int p,
    _In_ int r,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
MPI_Type_create_f90_integer(
    _In_ int r,
    _Out_ MPI_Datatype* newtype
    );

MPI_METHOD
PMPI_Type_create_f90_integer(
    _In_ int r,
    _Out_ MPI_Datatype* newtype
    );

/* typeclasses */
#define MPI_TYPECLASS_REAL      1
#define MPI_TYPECLASS_INTEGER   2
#define MPI_TYPECLASS_COMPLEX   3

MPI_METHOD
MPI_Type_match_size(
    _In_ int typeclass,
    _In_ int size,
    _Out_ MPI_Datatype* datatype
    );

MPI_METHOD
PMPI_Type_match_size(
    _In_ int typeclass,
    _In_ int size,
    _Out_ MPI_Datatype* datatype
    );


/*---------------------------------------------*/
/* Section 16.3: Language Interoperability     */
/*---------------------------------------------*/

#define MPI_Comm_c2f(comm)  (MPI_Fint)(comm)
#define PMPI_Comm_c2f(comm) (MPI_Fint)(comm)

#define MPI_Comm_f2c(comm)  (MPI_Comm)(comm)
#define PMPI_Comm_f2c(comm) (MPI_Comm)(comm)


#define MPI_Type_f2c(datatype)  (MPI_Datatype)(datatype)
#define PMPI_Type_f2c(datatype) (MPI_Datatype)(datatype)

#define MPI_Type_c2f(datatype)  (MPI_Fint)(datatype)
#define PMPI_Type_c2f(datatype) (MPI_Fint)(datatype)


#define MPI_Group_f2c(group)  (MPI_Group)(group)
#define PMPI_Group_f2c(group) (MPI_Group)(group)

#define MPI_Group_c2f(group)  (MPI_Fint)(group)
#define PMPI_Group_c2f(group) (MPI_Fint)(group)


#define MPI_Request_f2c(request)  (MPI_Request)(request)
#define PMPI_Request_f2c(request) (MPI_Request)(request)

#define MPI_Request_c2f(request)  (MPI_Fint)(request)
#define PMPI_Request_c2f(request) (MPI_Fint)(request)


#define MPI_Win_f2c(win)  (MPI_Win)(win)
#define PMPI_Win_f2c(win) (MPI_Win)(win)

#define MPI_Win_c2f(win)  (MPI_Fint)(win)
#define PMPI_Win_c2f(win) (MPI_Fint)(win)


#define MPI_Op_c2f(op)  (MPI_Fint)(op)
#define PMPI_Op_c2f(op) (MPI_Fint)(op)

#define MPI_Op_f2c(op)  (MPI_Op)(op)
#define PMPI_Op_f2c(op) (MPI_Op)(op)


#define MPI_Info_c2f(info)  (MPI_Fint)(info)
#define PMPI_Info_c2f(info) (MPI_Fint)(info)

#define MPI_Info_f2c(info)  (MPI_Info)(info)
#define PMPI_Info_f2c(info) (MPI_Info)(info)


#define MPI_Message_c2f(msg)  (MPI_Fint)(msg)
#define PMPI_Message_c2f(msg) (MPI_Fint)(msg)

#define MPI_Message_f2c(msg)  (MPI_Message)(msg)
#define PMPI_Message_f2c(msg) (MPI_Message)(msg)


#define MPI_Errhandler_c2f(errhandler)  (MPI_Fint)(errhandler)
#define PMPI_Errhandler_c2f(errhandler) (MPI_Fint)(errhandler)

#define MPI_Errhandler_f2c(errhandler)  (MPI_Errhandler)(errhandler)
#define PMPI_Errhandler_f2c(errhandler) (MPI_Errhandler)(errhandler)


MPI_File
MPIAPI
MPI_File_f2c(
    _In_ MPI_Fint file
    );

MPI_File
MPIAPI
PMPI_File_f2c(
    _In_ MPI_Fint file
    );

MPI_Fint
MPIAPI
MPI_File_c2f(
    _In_ MPI_File file
    );

MPI_Fint
MPIAPI
PMPI_File_c2f(
    _In_ MPI_File file
    );

MPI_METHOD
MPI_Status_f2c(
    _In_ const MPI_Fint* f_status,
    _Out_ MPI_Status* c_status
    );

MPI_METHOD
PMPI_Status_f2c(
    _In_ const MPI_Fint* f_status,
    _Out_ MPI_Status* c_status
    );

MPI_METHOD
MPI_Status_c2f(
    _In_ const MPI_Status* c_status,
    _Out_ MPI_Fint* f_status
    );

MPI_METHOD
PMPI_Status_c2f(
    _In_ const MPI_Status* c_status,
    _Out_ MPI_Fint* f_status
    );


#if !defined(_MPICH_DLL_)
#define MPIU_DLL_SPEC __declspec(dllimport)
#else
#define MPIU_DLL_SPEC
#endif

extern MPIU_DLL_SPEC MPI_Fint* MPI_F_STATUS_IGNORE;
extern MPIU_DLL_SPEC MPI_Fint* MPI_F_STATUSES_IGNORE;


/*---------------------------------------------------------------------------*/
/* Implementation Specific                                                   */
/*---------------------------------------------------------------------------*/

MPI_METHOD
MPIR_Dup_fn(
    _In_ MPI_Comm oldcomm,
    _In_ int keyval,
    _In_opt_ void* extra_state,
    _In_opt_ void* attribute_val_in,
    _Out_ void* attribute_val_out,
    _mpi_out_flag_ int* flag
    );


#if MSMPI_VER >= 0x300

MPI_METHOD
MSMPI_Get_bsend_overhead();

#endif


#if MSMPI_VER >= 0x300

MPI_METHOD
MSMPI_Get_version();

#else
#  define MSMPI_Get_version() (MSMPI_VER)
#endif

typedef void
(MPIAPI MSMPI_Request_callback)(
    _In_ MPI_Status* status
    );

MPI_METHOD
MSMPI_Request_set_apc(
    _In_ MPI_Request request,
    _In_ MSMPI_Request_callback* callback_fn,
    _In_ MPI_Status* callback_status
    );

typedef struct _MSMPI_LOCK_QUEUE
{
    struct _MSMPI_LOCK_QUEUE* volatile next;
    volatile MPI_Aint flags;

} MSMPI_Lock_queue;

void
MPIAPI
MSMPI_Queuelock_acquire(
    _Out_ MSMPI_Lock_queue* queue
    );

void
MPIAPI
MSMPI_Queuelock_release(
    _In_ MSMPI_Lock_queue* queue
    );

MPI_METHOD
MSMPI_Waitsome_interruptible(
    _In_range_(>=, 0) int incount,
    _Inout_updates_opt_(incount) MPI_Request array_of_requests[],
    _Out_ _Deref_out_range_(MPI_UNDEFINED, incount) int* outcount,
    _Out_writes_to_opt_(incount,*outcount) int array_of_indices[],
    _Out_writes_to_opt_(incount,*outcount) MPI_Status array_of_statuses[]
    );


#if defined(__cplusplus)
}
#endif

#endif /* MPI_INCLUDED */
