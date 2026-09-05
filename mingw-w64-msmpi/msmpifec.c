// mpif.c – minimal gfortran-compatible Fortran bindings for MS-MPI
//
// GFortran does not support DLLIMPORT on COMMON block variable. And even if it
// did, the COMMON block variables that are exported from msmpi.dll have Intel
// Fortran layout (which would be incompatible with GFortran anyway).
//
// To still support using the sentinel variables that msmpi.dll exports in Intel
// Fortran COMMON blocks, replace them in a wrapper using sentinel variables
// that are dllimported from a small helper DLL. These wrappers need to be
// linked with all (Fortran) projects linking to msmpi.dll in MSYS2.

#include <windows.h>
#include <mpi.h>

#define MSMPIF_API __attribute__ ((dllexport))

// ---------------------------------------------------------------------
// Global variables (Fortran module + mpif.h)
// ---------------------------------------------------------------------

// global variables dllimported by Fortran module
#if defined(__clang__)
// LLVM Flang name mangling
extern int _QMmpi_constantsEmpi_bottom;
extern int _QMmpi_constantsEmpi_in_place;
extern int _QMmpi_constantsEmpi_status_ignore[5];
extern int _QMmpi_constantsEmpi_statuses_ignore[5][1];
extern int _QMmpi_constantsEmpi_errcodes_ignore[1];
extern int _QMmpi_constantsEmpi_unweighted;
extern int _QMmpi_constantsEmpi_weights_empty;
extern int _QMmpi_constantsEmpi_argvs_null;
extern int _QMmpi_constantsEmpi_argv_null;

#define mod_mpi_bottom _QMmpi_constantsEmpi_bottom
#define mod_mpi_in_place _QMmpi_constantsEmpi_in_place
#define mod_mpi_status_ignore _QMmpi_constantsEmpi_status_ignore
#define mod_mpi_statuses_ignore _QMmpi_constantsEmpi_statuses_ignore
#define mod_mpi_errcodes_ignore _QMmpi_constantsEmpi_errcodes_ignore
#define mod_mpi_unweighted _QMmpi_constantsEmpi_unweighted
#define mod_mpi_weights_empty _QMmpi_constantsEmpi_weights_empty
#define mod_mpi_argvs_null _QMmpi_constantsEmpi_argvs_null
#define mod_mpi_argv_null _QMmpi_constantsEmpi_argv_null

#else
// GFortran name mangling
extern int __mpi_constants_MOD_mpi_bottom;
extern int __mpi_constants_MOD_mpi_in_place;
extern int __mpi_constants_MOD_mpi_status_ignore[5];
extern int __mpi_constants_MOD_mpi_statuses_ignore[5][1];
extern int __mpi_constants_MOD_mpi_errcodes_ignore[1];
extern int __mpi_constants_MOD_mpi_unweighted;
extern int __mpi_constants_MOD_mpi_weights_empty;
extern int __mpi_constants_MOD_mpi_argvs_null;
extern int __mpi_constants_MOD_mpi_argv_null;

#define mod_mpi_bottom __mpi_constants_MOD_mpi_bottom
#define mod_mpi_in_place __mpi_constants_MOD_mpi_in_place
#define mod_mpi_status_ignore __mpi_constants_MOD_mpi_status_ignore
#define mod_mpi_statuses_ignore __mpi_constants_MOD_mpi_statuses_ignore
#define mod_mpi_errcodes_ignore __mpi_constants_MOD_mpi_errcodes_ignore
#define mod_mpi_unweighted __mpi_constants_MOD_mpi_unweighted
#define mod_mpi_weights_empty __mpi_constants_MOD_mpi_weights_empty
#define mod_mpi_argvs_null __mpi_constants_MOD_mpi_argvs_null
#define mod_mpi_argv_null __mpi_constants_MOD_mpi_argv_null

#endif

// global variables dllimported by "mpif.h"
MSMPIF_API int mpi_bottom;
MSMPIF_API int mpi_in_place;
MSMPIF_API int mpi_status_ignore[5];
MSMPIF_API int mpi_statuses_ignore[5][1];
MSMPIF_API int mpi_errcodes_ignore[1];
MSMPIF_API int mpi_unweighted;
MSMPIF_API int mpi_weights_empty;
MSMPIF_API int mpi_argvs_null;
MSMPIF_API int mpi_argv_null;

// ---------------------------------------------------------------------
// Sentinel mapping helpers
// ---------------------------------------------------------------------

// Replace the Fortran sentinel values with their C counterpart.

static void* map_buf(void* arg)
{
  if (arg == &mod_mpi_in_place
      || arg == &mpi_in_place)
    return MPI_IN_PLACE;

  if (arg == &mod_mpi_bottom
      || arg == &mpi_bottom)
    return MPI_BOTTOM;

  return arg;
}

static MPI_Status* map_status(MPI_Status* arg)
{
  if ((int (*)[5])arg == &mod_mpi_status_ignore
      || (int (*)[5])arg == &mpi_status_ignore)
    return MPI_STATUS_IGNORE;

  return arg;
}

static MPI_Status* map_statuses(MPI_Status* arg)
{
  if ((int (*)[5][1])arg == &mod_mpi_statuses_ignore
      || (int (*)[5][1])arg == &mpi_statuses_ignore)
    return MPI_STATUSES_IGNORE;

  return arg;
}

static int *map_errcodes(int *arg)
{
  if ((int (*)[1])arg == &mod_mpi_errcodes_ignore
      || (int (*)[1])arg == &mpi_errcodes_ignore)
    return MPI_ERRCODES_IGNORE;

  return arg;
}

static int* map_weights(int* arg)
{
  if (arg == &mod_mpi_unweighted
      || arg == &mpi_unweighted)
    return MPI_UNWEIGHTED;

  if (arg == &mod_mpi_weights_empty
      || arg == &mpi_weights_empty)
    return MPI_WEIGHTS_EMPTY;

  return arg;
}

static char ***map_argvs(char ***arg)
{
  // Fortran passes the address of the module variable; compare that
  if ((int*)arg == &mod_mpi_argvs_null
      || (int*)arg == &mpi_argvs_null)
    return MPI_ARGVS_NULL;

  return arg;
}

static char **map_argv(char **arg)
{
  if ((int*)arg == &mod_mpi_argv_null
      || (int*)arg == &mpi_argv_null)
    return MPI_ARGV_NULL;

  return arg;
}

// ---------------------------------------------------------------------
// Dynamic loading of msmpi.dll
// ---------------------------------------------------------------------

static HMODULE hMsmpi = NULL;

static int load_msmpi(void)
{
  if (!hMsmpi)
    hMsmpi = GetModuleHandleA("msmpi.dll");
  if (!hMsmpi)
    hMsmpi = LoadLibraryA("msmpi.dll");
  return hMsmpi != NULL;
}

#define LOAD_MPI(name) \
  do { \
    if (! p##name) { \
      if (! load_msmpi()) return; \
      p##name = (name##_type) GetProcAddress(hMsmpi, #name); \
      if (!p##name) return; \
    } \
  } while (0)

// ---------------------------------------------------------------------
// C MPI function pointer typedefs
// ---------------------------------------------------------------------

typedef int (MPIAPI *MPI_Allreduce_type)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm);
typedef int (MPIAPI *MPI_Reduce_type)(const void*, void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
typedef int (MPIAPI *MPI_Reduce_scatter_type)(const void*, void*, const int*, MPI_Datatype, MPI_Op, MPI_Comm);
typedef int (MPIAPI *MPI_Reduce_scatter_block_type)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm);
typedef int (MPIAPI *MPI_Reduce_local_type)(const void*, void*, int, MPI_Datatype, MPI_Op);
typedef int (MPIAPI *MPI_Scan_type)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm);
typedef int (MPIAPI *MPI_Exscan_type)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm);

typedef int (MPIAPI *MPI_Gather_type)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);
typedef int (MPIAPI *MPI_Gatherv_type)(const void*, int, MPI_Datatype, void*, const int[], const int[], MPI_Datatype, int, MPI_Comm);
typedef int (MPIAPI *MPI_Allgather_type)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
typedef int (MPIAPI *MPI_Allgatherv_type)(const void*, int, MPI_Datatype, void*, const int[], const int[], MPI_Datatype, MPI_Comm);
typedef int (MPIAPI *MPI_Scatter_type)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);
typedef int (MPIAPI *MPI_Scatterv_type)(const void*, const int[], const int[], MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);
typedef int (MPIAPI *MPI_Alltoall_type)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
typedef int (MPIAPI *MPI_Alltoallv_type)(const void*, const int[], const int[], MPI_Datatype, void*, const int[], const int[], MPI_Datatype, MPI_Comm);
typedef int (MPIAPI *MPI_Alltoallw_type)(void*, const int[], const int[], const MPI_Datatype[], void*, const int[], const int[], const MPI_Datatype[], MPI_Comm);

typedef int (MPIAPI *MPI_Wait_type)(MPI_Request*, MPI_Status*);
typedef int (MPIAPI *MPI_Test_type)(MPI_Request*, int*, MPI_Status*);

typedef int (MPIAPI *MPI_Waitall_type)(int, MPI_Request[], MPI_Status[]);
typedef int (MPIAPI *MPI_Testall_type)(int, MPI_Request[], int*, MPI_Status[]);

typedef int (MPIAPI *MPI_Waitany_type)(int, MPI_Request[], int*, MPI_Status*);
typedef int (MPIAPI *MPI_Testany_type)(int, MPI_Request[], int*, int*, MPI_Status*);

typedef int (MPIAPI *MPI_Waitsome_type)(int, MPI_Request[], int*, int[], MPI_Status[]);
typedef int (MPIAPI *MPI_Testsome_type)(int, MPI_Request[], int*, int[], MPI_Status[]);

typedef int (MPIAPI *MPI_Comm_spawn_type)(const char *command, char *argv[], int maxprocs,
    MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm,
    int array_of_errcodes[]);
typedef int (MPIAPI *MPI_Comm_spawn_multiple_type)(int count,
    char *array_of_commands[], char **array_of_argv[], const int array_of_maxprocs[],
    const MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm,
    int array_of_errcodes[]);

typedef int (MPIAPI *MPI_Dist_graph_neighbors_type)(MPI_Comm comm,
    int maxindegree, int sources[], int sourceweights[],
    int maxoutdegree, int destinations[], int destweights[]);

// (Nonblocking variants – prototypes simplified to match MS-MPI C API)
typedef int (MPIAPI *MPI_Ireduce_type)(const void*, void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Iallreduce_type)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Ireduce_scatter_type)(const void*, void*, const int[], MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Ireduce_scatter_block_type)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Iscan_type)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Iexscan_type)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*);

typedef int (MPIAPI *MPI_Igather_type)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Igatherv_type)(const void*, int, MPI_Datatype, void*, const int[], const int[], MPI_Datatype, int, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Iallgather_type)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Iallgatherv_type)(const void*, int, MPI_Datatype, void*, const int[], const int[], MPI_Datatype, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Iscatter_type)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Iscatterv_type)(const void*, const int[], const int[], MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Ialltoall_type)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Ialltoallv_type)(const void*, const int[], const int[], MPI_Datatype, void*, const int[], const int[], MPI_Datatype, MPI_Comm, MPI_Request*);
typedef int (MPIAPI *MPI_Ialltoallw_type)(const void*, const int[], const int[], const MPI_Datatype[], void*, const int[], const int[], const MPI_Datatype[], MPI_Comm, MPI_Request*);

// ---------------------------------------------------------------------
// Function pointers
// ---------------------------------------------------------------------

static MPI_Allreduce_type pMPI_Allreduce = NULL;
static MPI_Reduce_type pMPI_Reduce = NULL;
static MPI_Reduce_scatter_type pMPI_Reduce_scatter = NULL;
static MPI_Reduce_scatter_block_type pMPI_Reduce_scatter_block = NULL;
static MPI_Reduce_local_type pMPI_Reduce_local = NULL;
static MPI_Scan_type pMPI_Scan = NULL;
static MPI_Exscan_type pMPI_Exscan = NULL;

static MPI_Gather_type pMPI_Gather = NULL;
static MPI_Gatherv_type pMPI_Gatherv = NULL;
static MPI_Allgather_type pMPI_Allgather = NULL;
static MPI_Allgatherv_type pMPI_Allgatherv = NULL;
static MPI_Scatter_type pMPI_Scatter = NULL;
static MPI_Scatterv_type pMPI_Scatterv = NULL;
static MPI_Alltoall_type pMPI_Alltoall = NULL;
static MPI_Alltoallv_type pMPI_Alltoallv = NULL;
static MPI_Alltoallw_type pMPI_Alltoallw = NULL;

static MPI_Wait_type pMPI_Wait = NULL;
static MPI_Test_type pMPI_Test = NULL;

static MPI_Waitall_type pMPI_Waitall = NULL;
static MPI_Testall_type pMPI_Testall = NULL;

static MPI_Waitany_type pMPI_Waitany = NULL;
static MPI_Testany_type pMPI_Testany = NULL;

static MPI_Waitsome_type pMPI_Waitsome = NULL;
static MPI_Testsome_type pMPI_Testsome = NULL;

static MPI_Comm_spawn_type pMPI_Comm_spawn = NULL;
static MPI_Comm_spawn_multiple_type pMPI_Comm_spawn_multiple = NULL;

static MPI_Dist_graph_neighbors_type pMPI_Dist_graph_neighbors = NULL;

static MPI_Ireduce_type pMPI_Ireduce = NULL;
static MPI_Iallreduce_type pMPI_Iallreduce = NULL;
static MPI_Ireduce_scatter_type pMPI_Ireduce_scatter = NULL;
static MPI_Ireduce_scatter_block_type pMPI_Ireduce_scatter_block = NULL;
static MPI_Iscan_type pMPI_Iscan = NULL;
static MPI_Iexscan_type pMPI_Iexscan = NULL;

static MPI_Igather_type pMPI_Igather = NULL;
static MPI_Igatherv_type pMPI_Igatherv = NULL;
static MPI_Iallgather_type pMPI_Iallgather = NULL;
static MPI_Iallgatherv_type pMPI_Iallgatherv = NULL;
static MPI_Iscatter_type pMPI_Iscatter = NULL;
static MPI_Iscatterv_type pMPI_Iscatterv = NULL;
static MPI_Ialltoall_type pMPI_Ialltoall = NULL;
static MPI_Ialltoallv_type pMPI_Ialltoallv = NULL;
static MPI_Ialltoallw_type pMPI_Ialltoallw = NULL;

// ---------------------------------------------------------------------
// Fortran wrappers (gfortran: lowercase + trailing underscore)
// ---------------------------------------------------------------------
//
// These functions replace the original ones from msmpi.dll when called from
// Fortran code compiled with GFortran.

// -------------------- Collective ops with IN_PLACE/BOTTOM --------------------

MSMPIF_API void
mpi_allreduce_(void *sendbuf, void *recvbuf,
               const int *count, const int *datatype, const int *op, const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Allreduce);

  *ierr = pMPI_Allreduce(
    map_buf(sendbuf), map_buf(recvbuf),
    *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op), (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_reduce_(void *sendbuf, void *recvbuf,
            const int *count, const int *datatype, const int *op, const int *root, const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Reduce);

  *ierr = pMPI_Reduce(
    map_buf(sendbuf), map_buf(recvbuf),
    *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op), *root, (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_reduce_scatter_(void *sendbuf, void *recvbuf,
                    const int *recvcounts, const int *datatype, const int *op, const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Reduce_scatter);

  *ierr = pMPI_Reduce_scatter(
    map_buf(sendbuf), map_buf(recvbuf),
    recvcounts, (MPI_Datatype)(*datatype), (MPI_Op)(*op), (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_reduce_scatter_block_(void *sendbuf, void *recvbuf,
                          const int *recvcount, const int *datatype, const int *op, const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Reduce_scatter_block);

  *ierr = pMPI_Reduce_scatter_block(
    map_buf(sendbuf), recvbuf,
    *recvcount, (MPI_Datatype)(*datatype), (MPI_Op)(*op), (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_reduce_local_(void *inbuf, void *inoutbuf,
                  int *count, int *datatype, int *op,
                  int *ierr)
{
  LOAD_MPI(MPI_Reduce_local);

  *ierr = pMPI_Reduce_local(
      map_buf(inbuf), map_buf(inoutbuf),
      *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op));
}

MSMPIF_API void
mpi_scan_(void *sendbuf, void *recvbuf,
          const int *count, const int *datatype, const int *op, const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Scan);

  *ierr = pMPI_Scan(
    map_buf(sendbuf), map_buf(recvbuf),
    *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op), (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_exscan_(void *sendbuf, void *recvbuf,
            const int *count, const int *datatype, const int *op, const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Exscan);

  *ierr = pMPI_Exscan(
    map_buf(sendbuf), map_buf(recvbuf),
    *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op), (MPI_Comm)(*comm));
}

// -------------------- Gather/scatter/alltoall with IN_PLACE --------------------

MSMPIF_API void
mpi_gather_(void *sendbuf, const int *sendcount, const int *sendtype,
            void *recvbuf, const int *recvcount, const int *recvtype,
            const int *root, const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Gather);

  *ierr = pMPI_Gather(
    map_buf(sendbuf), *sendcount, (MPI_Datatype)(*sendtype),
    map_buf(recvbuf), *recvcount, (MPI_Datatype)(*recvtype),
    *root, (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_gatherv_(void *sendbuf, const int *sendcount, const int *sendtype,
             void *recvbuf, const int *recvcounts, const int *displs, const int *recvtype,
             const int *root, const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Gatherv);

  *ierr = pMPI_Gatherv(
    map_buf(sendbuf), *sendcount, (MPI_Datatype)(*sendtype),
    map_buf(recvbuf), recvcounts, displs, (MPI_Datatype)(*recvtype),
    *root, (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_allgather_(void *sendbuf, const int *sendcount, const int *sendtype,
               void *recvbuf, const int *recvcount, const int *recvtype,
               const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Allgather);

  *ierr = pMPI_Allgather(
    map_buf(sendbuf), *sendcount, (MPI_Datatype)(*sendtype),
    map_buf(recvbuf), *recvcount, (MPI_Datatype)(*recvtype),
    (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_allgatherv_(void *sendbuf, const int *sendcount, const int *sendtype,
                void *recvbuf, const int *recvcounts, const int *displs, const int *recvtype,
                const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Allgatherv);

  *ierr = pMPI_Allgatherv(
    map_buf(sendbuf), *sendcount, (MPI_Datatype)(*sendtype),
    map_buf(recvbuf), recvcounts, displs, (MPI_Datatype)(*recvtype),
    (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_scatter_(void *sendbuf, const int *sendcount, const int *sendtype,
             void *recvbuf, const int *recvcount, const int *recvtype,
             const int *root, const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Scatter);

  *ierr = pMPI_Scatter(
    map_buf(sendbuf), *sendcount, (MPI_Datatype)(*sendtype),
    map_buf(recvbuf), *recvcount, (MPI_Datatype)(*recvtype),
    *root, (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_scatterv_(void *sendbuf, const int *sendcounts, const int *displs, const int *sendtype,
              void *recvbuf, const int *recvcount, const int *recvtype,
              const int *root, const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Scatterv);

  *ierr = pMPI_Scatterv(
    map_buf(sendbuf), sendcounts, displs, (MPI_Datatype)(*sendtype),
    map_buf(recvbuf), *recvcount, (MPI_Datatype)(*recvtype),
    *root, (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_alltoall_(void *sendbuf, const int *sendcount, const int *sendtype,
              void *recvbuf, const int *recvcount, const int *recvtype,
              const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Alltoall);

  *ierr = pMPI_Alltoall(
    map_buf(sendbuf), *sendcount, (MPI_Datatype)(*sendtype),
    map_buf(recvbuf), *recvcount, (MPI_Datatype)(*recvtype),
    (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_alltoallv_(void *sendbuf, const int *sendcounts, const int *sdispls, const int *sendtype,
               void *recvbuf, const int *recvcounts, const int *rdispls, const int *recvtype,
               const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Alltoallv);

  *ierr = pMPI_Alltoallv(
    map_buf(sendbuf), sendcounts, sdispls, (MPI_Datatype)(*sendtype),
    map_buf(recvbuf), recvcounts, rdispls, (MPI_Datatype)(*recvtype),
    (MPI_Comm)(*comm));
}

MSMPIF_API void
mpi_alltoallw_(const void *sendbuf, const int sendcounts[], const int sdispls[], const int sendtypes[],
               void *recvbuf, const int recvcounts[], const int rdispls[], const int recvtypes[],
               const int *comm, int *ierr)
{
  LOAD_MPI(MPI_Alltoallw);

  *ierr = pMPI_Alltoallw(
    map_buf((void *)sendbuf), sendcounts, sdispls, (MPI_Datatype *)sendtypes,
    map_buf(recvbuf), recvcounts, rdispls, (MPI_Datatype *)recvtypes,
    (MPI_Comm)(*comm));
}

// -------------------- Status-ignore wrappers --------------------

MSMPIF_API void
mpi_wait_(int *request, int *status, int *ierr)
{
  LOAD_MPI(MPI_Wait);

  *ierr = pMPI_Wait((MPI_Request *)request, map_status((MPI_Status *)status));
}

MSMPIF_API void
mpi_test_(int *request, int *flag, int *status, int *ierr)
{
  int c_flag;

  LOAD_MPI(MPI_Test);

  *ierr = pMPI_Test((MPI_Request *)request, &c_flag, map_status((MPI_Status *)status));

  *flag = (c_flag ? 1 : 0);
}

MSMPIF_API void
mpi_waitall_(const int *n, int *reqs, int *stats, int *ierr)
{
  LOAD_MPI(MPI_Waitall);

  *ierr = pMPI_Waitall(*n, (MPI_Request *)reqs, map_statuses((MPI_Status *)stats));
}

MSMPIF_API void
mpi_testall_(int *n, int *reqs, int *flag, int *stats, int *ierr)
{
  int c_flag;

  LOAD_MPI(MPI_Testall);

  *ierr = pMPI_Testall(*n, (MPI_Request *)reqs, &c_flag, map_statuses((MPI_Status *)stats));

  *flag = (c_flag ? 1 : 0);
}

MSMPIF_API void
mpi_waitany_(int *n, MPI_Request *reqs, int *index, MPI_Status *status, int *ierr)
{
  int c_index;

  LOAD_MPI(MPI_Waitany);

  *ierr = pMPI_Waitany(*n, reqs, &c_index, map_status(status));

  *index = c_index;
  // Fortran starts indexing with 1
  if (c_index >= 0)
    *index = *index + 1;
}

MSMPIF_API void
mpi_testany_(int *n, MPI_Request *reqs, int *index, int *flag, MPI_Status *status, int *ierr)
{
  int c_index;
  int c_flag;

  LOAD_MPI(MPI_Testany);

  *ierr = pMPI_Testany(*n, reqs, &c_index, &c_flag, map_status(status));

  *index = c_index;
  // Fortran starts indexing with 1
  if (c_index >= 0)
    *index = *index + 1;

  *flag = (c_flag ? 1 : 0);
}

MSMPIF_API void
mpi_waitsome_(int *n, MPI_Request *reqs, int *outcount, int *indices, MPI_Status *stats, int *ierr)
{
  LOAD_MPI(MPI_Waitsome);

  *ierr = pMPI_Waitsome(*n, reqs, outcount, indices, map_statuses(stats));

  // Fortran starts indexing with 1
  for (int li = 0; li < *outcount; li++)
  {
    if (indices[li] >= 0)
      indices[li] += 1;
  }
}

MSMPIF_API void
mpi_testsome_(int *n, MPI_Request *reqs, int *outcount, int *indices, MPI_Status *stats, int *ierr)
{
  LOAD_MPI(MPI_Testsome);

  *ierr = pMPI_Testsome(*n, reqs, outcount, indices, map_statuses(stats));

  // Fortran starts indexing with 1
  for (int li = 0; li < *outcount; li++)
  {
    if (indices[li] >= 0)
      indices[li] += 1;
  }
}

// -------------------- spawn wrappers (ARGV/ARGVS + ERR CODES) --------------------

MSMPIF_API void
mpi_comm_spawn_(const char *command, const char *argv_block, const int *maxprocs,
    const int *info, const int *root, const int *comm, int *intercomm,
    int array_of_errcodes[], int *ierr,
    int d1, int d2)
{
  char *cmd = NULL;
  char **argv = NULL;
  char *argv_storage = NULL;
  int argc = 0;

  LOAD_MPI(MPI_Comm_spawn);

  /* ---------------------------
     Trim and copy COMMAND
     --------------------------- */
  {
    const char *p = command + d1 - 1;
    while (p > command && *p == ' ')
      p--;
    int len = (int)(p - command + 1);

    cmd = (char *)malloc(len + 1);
    if (!cmd)
    {
      *ierr = MPI_ERR_NO_MEM;
      goto cleanup;
    }
    memcpy(cmd, command, len);
    cmd[len] = '\0';
  }

  /* ---------------------------
     Build ARGV array for C interface
     Fortran passes a flat block:
       argv_block = [entry0][entry1][entry2]...
     Each entry is CHARACTER(d2)
     MS-MPI terminates when an entry is all blanks.
     --------------------------- */
  {
    const char *p = argv_block;

    /* Count entries until all-blank */
    for (argc = 0;; argc++)
    {
      const char *end = p + d2 - 1;
      while (end > p && *end == ' ')
        end--;
      if (*end == ' ')  /* all blank */
        break;
      p += d2;
    }

    argv = (char **)malloc((argc + 1) * sizeof(char *));
    if (!argv)
    {
      *ierr = MPI_ERR_NO_MEM;
      goto cleanup;
    }

    argv_storage = (char *)malloc(argc * (d2 + 1));
    if (!argv_storage)
    {
      *ierr = MPI_ERR_NO_MEM;
      goto cleanup;
    }

    for (int i = 0; i < argc; i++)
    {
      const char *src = argv_block + i * d2;
      const char *end = src + d2 - 1;
      while (end > src && *end == ' ')
        end--;
      int len = (int)(end - src + 1);

      char *dest = argv_storage + i * (d2 + 1);
      memcpy(dest, src, len);
      dest[len] = '\0';
      argv[i] = dest;
    }

    argv[argc] = NULL;   /* null-terminate array */
  }

  /* ---------------------------
     Call C MPI_Comm_spawn
     --------------------------- */
  *ierr = pMPI_Comm_spawn(cmd, argv, *maxprocs,
      (MPI_Info)(*info), *root, (MPI_Comm)(*comm), (MPI_Comm *)intercomm,
      map_errcodes(array_of_errcodes));

cleanup:
  if (argv_storage)
    free(argv_storage);
  if (argv)
    free(argv);
  if (cmd)
    free(cmd);
}

MSMPIF_API void
mpi_comm_spawn_multiple_(const int *count,
    const char *commands_block, const char *argv_block, const int maxprocs[],
    const int infos[], const int *root, const int *comm, int *intercomm,
    int array_of_errcodes[], int *ierr,
    int d2, int d3)
{
  char **commands = NULL;
  char *commands_storage = NULL;
  char ***argv = NULL;
  char **argv_storage = NULL; /* per-command storage base pointers */
  int ncmd = *count;

  LOAD_MPI(MPI_Comm_spawn_multiple);

  /* ---------------------------
     Build COMMANDS array
     Fortran passes flat block:
       commands_block = [cmd0][cmd1]...[cmdN-1]
     Each entry is CHARACTER(d2)
     --------------------------- */
  {
    int asize = ncmd + 1; /* extra NULL terminator */

    commands = (char **)malloc(asize * sizeof(char *));
    if (!commands)
    {
      *ierr = MPI_ERR_NO_MEM;
      goto cleanup;
    }

    commands_storage = (char *)malloc(asize * (d2 + 1));
    if (!commands_storage)
    {
      *ierr = MPI_ERR_NO_MEM;
      goto cleanup;
    }

    for (int i = 0; i < ncmd; i++)
    {
      const char *src = commands_block + i * d2;
      const char *end = src + d2 - 1;
      while (end > src && *end == ' ')
        end--;
      int len = (int)(end - src + 1);

      char *dest = commands_storage + i * (d2 + 1);
      memcpy(dest, src, len);
      dest[len] = '\0';
      commands[i] = dest;
    }

    /* Null terminate the array */
    commands[ncmd] = NULL;
  }

  /* ---------------------------
     Build ARGV array-of-arrays
     Fortran passes a 2D block:
       argv_block(k, i) with CHARACTER(d3)
     laid out column-major:
       row k starts at argv_block + k*d3
       next arg for same command is + (*count)*d3
     Each row is terminated by an all-blank entry.
     --------------------------- */
  {
    argv = (char ***)malloc(ncmd * sizeof(char **));
    if (!argv)
    {
      *ierr = MPI_ERR_NO_MEM;
      goto cleanup;
    }

    argv_storage = (char **)malloc(ncmd * sizeof(char *));
    if (!argv_storage)
    {
      *ierr = MPI_ERR_NO_MEM;
      goto cleanup;
    }

    for (int k = 0; k < ncmd; k++)
    {
      const char *p = argv_block + k * d3;
      int argc = 0;

      /* Count arguments until all-blank entry */
      for (;;)
      {
        const char *end = p + d3 - 1;
        while (end > p && *end == ' ')
          end--;
        if (*end == ' ' && end == p)
          break; /* all blank => terminator */

        argc++;
        p += ncmd * d3;
      }

      /* Allocate pointers and storage for this command's args */
      char **pargs = (char **)malloc((argc + 1) * sizeof(char *));
      if (!pargs)
      {
        *ierr = MPI_ERR_NO_MEM;
        goto cleanup;
      }

      char *pdata = (char *)malloc(argc * (d3 + 1));
      if (!pdata)
      {
        free(pargs);
        *ierr = MPI_ERR_NO_MEM;
        goto cleanup;
      }

      argv[k] = pargs;
      argv_storage[k] = pdata;

      /* Copy each argument, trimming and null-terminating */
      p = argv_block + k * d3;
      for (int i = 0; i < argc; i++)
      {
        const char *src = p;
        const char *end = src + d3 - 1;
        while (end > src && *end == ' ')
          end--;
        int len = (int)(end - src + 1);

        char *dest = pdata + i * (d3 + 1);
        memcpy(dest, src, len);
        dest[len] = '\0';

        pargs[i] = dest;
        p += ncmd * d3;
      }

      pargs[argc] = NULL; /* terminate argv[k] */
    }
  }

  /* ---------------------------
     Call C MPI_Comm_spawn_multiple
     --------------------------- */
  *ierr = pMPI_Comm_spawn_multiple(*count, commands, argv, maxprocs,
      (MPI_Info *)infos, *root, (MPI_Comm)(*comm), (MPI_Comm *)intercomm,
      map_errcodes(array_of_errcodes));

cleanup:
  if (argv)
  {
    for (int k = 0; k < ncmd; k++)
    {
      if (argv_storage && argv_storage[k])
        free(argv_storage[k]);
      if (argv[k])
        free(argv[k]);
    }
    free(argv);
  }
  if (argv_storage)
    free(argv_storage);
  if (commands_storage)
    free(commands_storage);
  if (commands)
    free(commands);
}

/*

// This function was proposed to become part of MPI. It is unclear whether
// it is actually implemented in some versions of MS-MPI.

// -------------------- cart_weighted_create wrapper (UNWEIGHTED/WEIGHTS_EMPTY) --------------------

MSMPIF_API void
mpi_cart_weighted_create_(int *comm_old, int *ndims, int *dims, int *weights,
    int *comm_cart, int *ierr)
{
  LOAD_MPI(MPI_Cart_weighted_create);

  *ierr = pMPI_Cart_weighted_create(
      (MPI_Comm)(*comm_old), *ndims, dims, map_weights(weights),
      (MPI_Comm*)comm_cart);
}
*/

// -------------------- dist_graph_neighbors wrapper --------------------

MSMPIF_API void
mpi_dist_graph_neighbors_(int *comm,
    int *maxindegree, int *sources, int *sourceweights,
    int *maxoutdegree, int *destinations, int *destweights,
    int *ierr)
{
  LOAD_MPI(MPI_Dist_graph_neighbors);

  *ierr = pMPI_Dist_graph_neighbors(
      (MPI_Comm)*comm,
      *maxindegree, sources, map_weights(sourceweights),
      *maxoutdegree, destinations, map_weights(destweights));
}

// -------------------- nonblocking reduction-style collectives with IN_PLACE --------------------

MSMPIF_API void
mpi_ireduce_(void *sendbuf, void *recvbuf,
             const int *count, const int *datatype, const int *op,
             const int *root, const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Ireduce);

  *ierr = pMPI_Ireduce(
      map_buf(sendbuf), map_buf(recvbuf),
      *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op),
      *root, (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_iallreduce_(void *sendbuf, void *recvbuf,
                const int *count, const int *datatype, const int *op,
                const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Iallreduce);

  *ierr = pMPI_Iallreduce(
      map_buf(sendbuf), map_buf(recvbuf),
      *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op),
      (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_ireduce_scatter_(void *sendbuf, void *recvbuf,
                     const int *recvcounts, const int *datatype, const int *op,
                     const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Ireduce_scatter);

  *ierr = pMPI_Ireduce_scatter(
      map_buf(sendbuf), map_buf(recvbuf),
      recvcounts, (MPI_Datatype)(*datatype), (MPI_Op)(*op),
      (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_ireduce_scatter_block_(void *sendbuf, void *recvbuf,
                           const int *recvcount, const int *datatype, const int *op,
                           const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Ireduce_scatter_block);

  *ierr = pMPI_Ireduce_scatter_block(
      map_buf(sendbuf), map_buf(recvbuf),
      *recvcount, (MPI_Datatype)(*datatype), (MPI_Op)(*op),
      (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_iscan_(void *sendbuf, void *recvbuf,
           const int *count, const int *datatype, const int *op,
           const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Iscan);

  *ierr = pMPI_Iscan(
      map_buf(sendbuf), map_buf(recvbuf),
      *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op),
      (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_iexscan_(void *sendbuf, void *recvbuf,
             const int *count, const int *datatype, const int *op,
             const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Iexscan);

  *ierr = pMPI_Iexscan(
      map_buf(sendbuf), map_buf(recvbuf),
      *count, (MPI_Datatype)(*datatype), (MPI_Op)(*op),
      (MPI_Comm)(*comm), (MPI_Request *)request);
}

// -------------------- nonblocking gather/scatter/alltoall with IN_PLACE --------------------

MSMPIF_API void
mpi_igather_(void *sendbuf, const int *sendcount, const int *sendtype,
             void *recvbuf, const int *recvcount, const int *recvtype,
             const int *root, const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Igather);

  *ierr = pMPI_Igather(
      map_buf(sendbuf), *sendcount, (MPI_Datatype)(*sendtype),
      map_buf(recvbuf), *recvcount, (MPI_Datatype)(*recvtype),
      *root, (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_igatherv_(void *sendbuf, const int *sendcount, const int *sendtype,
              void *recvbuf, const int *recvcounts, const int *displs, const int *recvtype,
              const int *root, const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Igatherv);

  *ierr = pMPI_Igatherv(
      map_buf(sendbuf), *sendcount, (MPI_Datatype)(*sendtype),
      map_buf(recvbuf), recvcounts, displs, (MPI_Datatype)(*recvtype),
      *root, (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_iallgather_(void *sendbuf, const int *sendcount, const int *sendtype,
                void *recvbuf, const int *recvcount, const int *recvtype,
                const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Iallgather);

  *ierr = pMPI_Iallgather(
      map_buf(sendbuf), *sendcount, (MPI_Datatype)*sendtype,
      map_buf(recvbuf), *recvcount, (MPI_Datatype)*recvtype,
      (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_iallgatherv_(void *sendbuf, const int *sendcount, const int *sendtype,
                 void *recvbuf, const int *recvcounts, const int *displs, const int *recvtype,
                 const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Iallgatherv);

  *ierr = pMPI_Iallgatherv(
      map_buf(sendbuf), *sendcount, (MPI_Datatype)(*sendtype),
      map_buf(recvbuf), recvcounts, displs, (MPI_Datatype)(*recvtype),
      (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_iscatter_(void *sendbuf, const int *sendcount, const int *sendtype,
              void *recvbuf, const int *recvcount, const int *recvtype,
              const int *root, const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Iscatter);

  *ierr = pMPI_Iscatter(
      map_buf(sendbuf), *sendcount, (MPI_Datatype)(*sendtype),
      map_buf(recvbuf), *recvcount, (MPI_Datatype)(*recvtype),
      *root, (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_iscatterv_(void *sendbuf, const int *sendcounts, const int *displs, const int *sendtype,
               void *recvbuf, const int *recvcount, const int *recvtype,
               const int *root, const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Iscatterv);

  *ierr = pMPI_Iscatterv(
      map_buf(sendbuf), sendcounts, displs, (MPI_Datatype)(*sendtype),
      map_buf(recvbuf), *recvcount, (MPI_Datatype)(*recvtype),
      *root, (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_ialltoall_(void *sendbuf, const int *sendcount, const int *sendtype,
               void *recvbuf, const int *recvcount, const int *recvtype,
               const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Ialltoall);

  *ierr = pMPI_Ialltoall(
      map_buf(sendbuf), *sendcount, (MPI_Datatype)(*sendtype),
      map_buf(recvbuf), *recvcount, (MPI_Datatype)(*recvtype),
      (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_ialltoallv_(void *sendbuf, const int *sendcounts, const int *sdispls, const int *sendtype,
                void *recvbuf, const int *recvcounts, const int *rdispls, const int *recvtype,
                const int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Ialltoallv);

  *ierr = pMPI_Ialltoallv(
      map_buf(sendbuf), sendcounts, sdispls, (MPI_Datatype)(*sendtype),
      map_buf(recvbuf), recvcounts, rdispls, (MPI_Datatype)(*recvtype),
      (MPI_Comm)(*comm), (MPI_Request *)request);
}

MSMPIF_API void
mpi_ialltoallw_(const void *sendbuf, const int sendcounts[], const int sdispls[], const int sendtypes[],
                void *recvbuf, const int recvcounts[], const int rdispls[], const int recvtypes[],
                int *comm, int *request, int *ierr)
{
  LOAD_MPI(MPI_Ialltoallw);

  *ierr = pMPI_Ialltoallw(
      map_buf((void *)sendbuf), sendcounts, sdispls, sendtypes,
      map_buf(recvbuf), recvcounts, rdispls, recvtypes,
      (MPI_Comm)(*comm), (MPI_Request *)request);
}
