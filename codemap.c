#define HASH_PRIME 49157
#define HASH(X) ((X)%HASH_PRIME)

typedef struct bucket_t {
    struct bucket_t* next;
    uint32_t address;
    uint32_t use_count;
    uint16_t* code;
} bucket_t;

static bucket_t* hash_buckets[HASH_PRIME] = { 0 };

static bool garbage_collect() {
    uint32_t removed = 0;                         // track how many we've freed
    uint32_t lru = 1<<31;                         // start lru at max int
    
    for(i=0; i<HASH_PRIME; i++) {                 // find LOWEST useage count first
        bucket_t* bucket = hash_buckets[i];
        while(bucket) {
            if(use_count < lru) {
                lru = use_count;
                if(lru == 1) break;               // exit early, lru cannot be < 1
            }
            bucket = bucket->next;
        }
        if(lru == 1) break;                       // exit early, lru cannot be < 1
    }
    
    for(i=0; i<HASH_PRIME; i++) {                 // removed zeroed entries
        bucket_t* bucket = hash_buckets[i];
        if(bucket) {                              // Handle head first
            bucket_t* next = bucket->next;
            if(!(bucket->use_count -= lru)) {     // If the count is zeroed
                hash_buckets[i]=next;             // Then we'll remove it and
                free(bucket->code);               // free all the memory used
                free(bucket);
                removed++;                        // and track that we did this
                                                  // it might be sufficient to exit here
            }
        }
        while(next) {                             // remaining items use loof-forward
            if((next->use_count -= lru)) {        // still has a high use_count
                bucket = next;                    // so we'll advance to the next
            } else {
                bucket->next = next->next;        // found another one to free
                free(next->code)                  // so free all the memory again
                free(next);
                removed++;                        // and track thia
            }
            next = bucket->next;                  // advance the look-ahead pointer
                                                  // it might be sufficient to exit here
        }
    }
    if(!removed) exit(1);                         // if nothing to remove, then die
    return true;                                  // always return true if we did
}

static bucket_t* find_bucket(uint32_t address) {
    uint16_t hash = HASH(address);
    bucket_t* bucket = hash_buckets[hash];
    while(bucket) {
        if(bucket->address == address) {            
            bucket->use_count++;                  // we're using it EVEN MORE!!!
            break;                                // found it, let's get outta here
        }                                         
        bucket = bucket->next;                    // try the next time (this should be short)
    }
    return bucket;
}

uint16_t* get_code_ptr(uint32_t address) {
    bucket_t* bucket = find_bucket(address);
    if(!bucket) return NULL;
    return bucker->code;
}

void set_code_ptr(uint32_t address, uint16_t* code) {
    bucket_t* bucket = find_bucket(address);
    if(!bucket) {
        do bucket = malloc(sizeof(bucket_t));     // try to create new entry
        while (!bucket && garbage_collect());     // and try, try again
        bucket->next = hash_buckets[hash];        // capture previous head as next
        hash_buckets[hash] = bucket;              // link into the head
        bucket->address = address;                // assign our entry point
    }
    else if(bucket->code) free(bucket->code);
    bucket->use_count = 1;                        // we're using it now
    bucket->code = code;
    return;
}

void free_code_ptrs(void) {
    for(i=0; i<HASH_PRIME; i++) {                 // removed zeroed entries
        bucket_t* bucket = hash_buckets[i];
        while(bucket) {
            bucket_t* next = bucket->next;
            free(bucket->code);
            free(bucket);
            bucket = next;
        }
    }
}
