#include "audioDB.h"
extern "C" {
#include "audioDB_API.h"
#include "audioDB-internals.h"
}

static bool audiodb_check_header(adb_header_t *header) {
  /* FIXME: use syslog() or write to stderr or something to give the
     poor user some diagnostics. */
  if(header->magic == O2_OLD_MAGIC) {
    return false;
  }
  if(header->magic != O2_MAGIC) {
    return false;
  }
  if(header->version != O2_FORMAT_VERSION) {
    return false;
  }
  if(header->headerSize != O2_HEADERSIZE) {
    return false;
  }
  return true;
}

static int audiodb_collect_keys(adb_t *adb) {
  char *key_table = 0;
  size_t key_table_length = 0;

  if(adb->header->length > 0) {
    unsigned nfiles = adb->header->numFiles;
    key_table_length = ALIGN_PAGE_UP(nfiles * O2_FILETABLE_ENTRY_SIZE);
    mmap_or_goto_error(char *, key_table, adb->header->fileTableOffset, key_table_length);
    for (unsigned int k = 0; k < nfiles; k++) {
      adb->keys->push_back(key_table + k*O2_FILETABLE_ENTRY_SIZE);
      (*adb->keymap)[(key_table + k*O2_FILETABLE_ENTRY_SIZE)] = k;
    }
    munmap(key_table, key_table_length);
  }

  return 0;

 error:
  maybe_munmap(key_table, key_table_length);
  return 1;
}

static int audiodb_collect_track_lengths(adb_t *adb) {
  uint32_t *track_table = 0;
  size_t track_table_length = 0;
  if(adb->header->length > 0) {
    unsigned nfiles = adb->header->numFiles;
    track_table_length = ALIGN_PAGE_UP(nfiles * O2_TRACKTABLE_ENTRY_SIZE);
    mmap_or_goto_error(uint32_t *, track_table, adb->header->trackTableOffset, track_table_length);
    off_t offset = 0;
    for (unsigned int k = 0; k < nfiles; k++) {
      uint32_t track_length = track_table[k];
      adb->track_lengths->push_back(track_length);
      adb->track_offsets->push_back(offset);
      offset += track_length * adb->header->dim * sizeof(double);
    }
    munmap(track_table, track_table_length);
  }

  return 0;

 error:
  maybe_munmap(track_table, track_table_length);
  return 1;
}

adb_t *audiodb_open(const char *path, int flags) {
  adb_t *adb = 0;
  int fd = -1;

  flags &= (O_RDONLY|O_RDWR);
  fd = open(path, flags);
  if(fd == -1) {
    goto error;
  }
  if(acquire_lock(fd, flags == O_RDWR)) {
    goto error;
  }

  adb = (adb_t *) calloc(1, sizeof(adb_t));
  if(!adb) {
    goto error;
  }
  adb->fd = fd;
  adb->flags = flags;
  adb->path = (char *) malloc(1+strlen(path));
  if(!(adb->path)) {
    goto error;
  }
  strcpy(adb->path, path);

  adb->header = (adb_header_t *) malloc(sizeof(adb_header_t));
  if(!(adb->header)) {
    goto error;
  }
  if(read(fd, (char *) adb->header, O2_HEADERSIZE) != O2_HEADERSIZE) {
    goto error;
  }
  if(!audiodb_check_header(adb->header)) {
    goto error;
  }

  adb->keys = new std::vector<std::string>;
  if(!adb->keys) {
    goto error;
  }
  adb->keymap = new std::map<std::string,uint32_t>;
  if(!adb->keymap) {
    goto error;
  }
  if(audiodb_collect_keys(adb)) {
    goto error;
  }
  adb->track_lengths = new std::vector<uint32_t>;
  if(!adb->track_lengths) {
    goto error;
  }
  adb->track_lengths->reserve(adb->header->numFiles);
  adb->track_offsets = new std::vector<off_t>;
  if(!adb->track_offsets) {
    goto error;
  }
  adb->track_offsets->reserve(adb->header->numFiles);
  if(audiodb_collect_track_lengths(adb)) {
    goto error;
  }
  adb->cached_lsh = 0;
  return adb;

 error:
  if(adb) {
    if(adb->header) {
      free(adb->header);
    }
    if(adb->path) {
      free(adb->path);
    }
    if(adb->keys) {
      delete adb->keys;
    }
    if(adb->keymap) {
      delete adb->keymap;
    }
    if(adb->track_lengths) {
      delete adb->track_lengths;
    }
    free(adb);
  }
  if(fd != -1) {
    close(fd);
  }
  return NULL;
}