#ifndef AUDIODB_API_H
#define AUDIODB_API_H

#include <stdbool.h>
#include <stdint.h>

/* for API questions contact 
 * Christophe Rhodes c.rhodes@gold.ac.uk
 * Ian Knopke mas01ik@gold.ac.uk, ian.knopke@gmail.com */

/* Temporary workarounds */
typedef struct dbTableHeader adb_header_t;
int acquire_lock(int, bool);
int divest_lock(int);


/*******************************************************************/
/* Data types for API */

/* The main struct that stores the name of the database, and in future will hold all
 * kinds of other interesting information */
/* This basically gets passed around to all of the other functions */

/* FIXME: it might be that "adb_" isn't such a good prefix to use, and
   that we should prefer "audiodb_".  Or else maybe we should be
   calling ourselves libadb? */
typedef struct adb adb_t, *adb_ptr;

struct adb_datum {
  uint32_t nvectors;
  uint32_t dim;
  const char *key;
  double *data;
  double *power;
  double *times;
};
typedef struct adb_datum adb_datum_t;

//used for both insert and batchinsert
struct adbinsert {
  const char *features;
  const char *power;
  const char *key;
  const char *times;
};
typedef struct adbinsert adb_insert_t, adb_reference_t, *adb_insert_ptr;

/* struct for returning status results */
struct adbstatus {
    unsigned int numFiles;  
    unsigned int dim;
    unsigned int dudCount;
    unsigned int nullCount;
    unsigned int flags;
    uint64_t length;
    uint64_t data_region_size;
};
typedef struct adbstatus adb_status_t, *adb_status_ptr;

/* needed for constructing a query */
struct adbquery {

    char * querytype;
    char * feature; //usually a file of some kind
    char * key;
    char * power; //also a file
    char * keylist; //also a file
    char * qpoint;  //position 
    char * numpoints;
    char * radius; 
    char * resultlength; //how many results to make
    char * sequencelength; 
    char * sequencehop; 
    double absolute_threshold; 
    double relative_threshold;
    int exhaustive; //hidden option in gengetopt
    double expandfactor; //hidden
    int rotate; //hidden

};
typedef struct adbquery adb_query_t,*adb_query_ptr;

/* ... and for getting query results back */
struct adbqueryresult {

    int sizeRlist; /* do I really need to return all 4 sizes here */
    int sizeDist;
    int sizeQpos;
    int sizeSpos;
    char **Rlist;
    double *Dist;
    unsigned int *Qpos;
    unsigned int *Spos;

};
typedef struct adbqueryresult adb_queryresult_t, *adb_queryresult_ptr;

/* New ("new" == December 2008) query API */

typedef struct adbresult {
  const char *key;
  double dist;
  uint32_t qpos;
  uint32_t ipos;
} adb_result_t;

#define ADB_REFINE_INCLUDE_KEYLIST 1
#define ADB_REFINE_EXCLUDE_KEYLIST 2
#define ADB_REFINE_RADIUS 4
#define ADB_REFINE_ABSOLUTE_THRESHOLD 8
#define ADB_REFINE_RELATIVE_THRESHOLD 16
#define ADB_REFINE_DURATION_RATIO 32
#define ADB_REFINE_HOP_SIZE 64

typedef struct adbkeylist {
  uint32_t nkeys;
  const char **keys;
} adb_keylist_t;

typedef struct adbqueryrefine {
  uint32_t flags;
  adb_keylist_t include;
  adb_keylist_t exclude;
  double radius;
  double absolute_threshold;
  double relative_threshold;
  double duration_ratio; /* expandfactor */
  uint32_t hopsize;
} adb_query_refine_t;

#define ADB_ACCUMULATION_DB 1
#define ADB_ACCUMULATION_PER_TRACK 2
#define ADB_ACCUMULATION_ONE_TO_ONE 3

#define ADB_DISTANCE_DOT_PRODUCT 1
#define ADB_DISTANCE_EUCLIDEAN_NORMED 2
#define ADB_DISTANCE_EUCLIDEAN 3

typedef struct adbqueryparameters {
  uint32_t accumulation;
  uint32_t distance;
  uint32_t npoints;
  uint32_t ntracks;
} adb_query_parameters_t;

typedef struct adbqueryresults {
  uint32_t nresults;
  adb_result_t *results;
} adb_query_results_t;

#define ADB_QID_FLAG_EXHAUSTIVE 1
#define ADB_QID_FLAG_ALLOW_FALSE_POSITIVES 2

typedef struct adbqueryid {
  adb_datum_t *datum;
  uint32_t sequence_length;
  uint32_t flags;
  uint32_t sequence_start;
} adb_query_id_t;

typedef struct adbqueryspec {
  adb_query_id_t qid;
  adb_query_parameters_t params;
  adb_query_refine_t refine;
} adb_query_spec_t;

/*******************************************************************/
/* Function prototypes for API */

/* open an existing database */
/* returns a struct or NULL on failure */
adb_ptr audiodb_open(const char *path, int flags);

/* create a new database */
/* returns a struct or NULL on failure */
adb_ptr audiodb_create(const char *path, unsigned datasize, unsigned ntracks, unsigned datadim);

/* close a database */
void audiodb_close(adb_ptr db);

/* You'll need to turn both of these on to do anything useful */
int audiodb_l2norm(adb_ptr mydb);
int audiodb_power(adb_ptr mydb);

/* insert functions */
int audiodb_insert_datum(adb_t *, const adb_datum_t *);
int audiodb_insert_reference(adb_t *, const adb_reference_t *);
int audiodb_insert(adb_ptr mydb, adb_insert_ptr ins);
int audiodb_batchinsert(adb_ptr mydb, adb_insert_ptr ins, unsigned int size);

/* query function */
int audiodb_query(adb_ptr mydb, adb_query_ptr adbq, adb_queryresult_ptr adbqres);
adb_query_results_t *audiodb_query_spec(adb_t *, const adb_query_spec_t *);
int audiodb_query_free_results(adb_t *, const adb_query_spec_t *, adb_query_results_t *);

/* database status */  
int audiodb_status(adb_ptr mydb, adb_status_ptr status);

/* varoius dump formats */
int audiodb_dump(adb_ptr mydb, const char *outputdir);

#endif
