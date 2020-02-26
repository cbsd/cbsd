/* credis.h -- a C client library for Redis, public API.
 *
 * Copyright (c) 2009-2010, Jonas Romfelt <jonas at romfelt dot se>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Credis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CREDIS_H
#define __CREDIS_H

#ifdef __cplusplus
extern "C" {
#endif

#define CR_BUFFER_SIZE 4096
#define CR_BUFFER_WATERMARK ((CR_BUFFER_SIZE)/10+1)
#define CR_MULTIBULK_SIZE 256

#define CR_ERROR '-'
#define CR_INLINE '+'
#define CR_BULK '$'
#define CR_MULTIBULK '*'
#define CR_INT ':'



typedef struct _cr_buffer {
  char *data;
  int idx;
  int len;
  int size;
} cr_buffer;

typedef struct _cr_multibulk {
  char **bulks;
  int *idxs;
  int size;
  int len;
} cr_multibulk;

typedef struct _cr_reply {
  int integer;
  char *line;
  char *bulk;
  cr_multibulk multibulk;
} cr_reply;

typedef struct _cr_redis {
  struct {
    int major;
    int minor;
    int patch;
  } version;
  int fd;
  char *ip;
  int port;
  int timeout;
  cr_buffer buf;
  cr_reply reply;
  int error;
} cr_redis;



/*
 * Functions list below is modelled after the Redis Command Reference (except
 * for the credis_connect() and credis_close() functions), use this reference 
 * for further descriptions of each command:
 *
 *    http://code.google.com/p/redis/wiki/CommandReference
 *
 * Comments are only available when it is not obvious how Credis implements 
 * the Redis command. In general, functions return 0 on success or a negative
 * value on error. Refer to CREDIS_ERR_* codes. The return code -1 is 
 * typically used when for instance a key is not found. 
 *
 * IMPORTANT! Memory buffers are allocated, used and managed by credis 
 * internally. Subsequent calls to credis functions _will_ destroy the data 
 * to which returned values reference to. If for instance the returned value 
 * by a call to credis_get() is to be used later in the program, a strdup() 
 * is highly recommended. However, each `REDIS' handle has its own state and 
 * manages its own memory buffers independently. That means that one of two 
 * handles can be destroyed while the other keeps its connection and data.
 * 
 * EXAMPLE
 * 
 * Connect to a Redis server and set value of key `fruit' to `banana': 
 *
 *    REDIS rh = credis_connect("localhost", 6789, 2000);
 *    credis_set(rh, "fruit", "banana");
 *    credis_close(rh);
 *
 * TODO
 *
 *  - Add support for missing Redis commands marked as TODO below
 *  - Currently only support for zero-terminated strings, not for storing 
 *    abritary binary data as bulk data. Basically an API issue since it is 
 *    partially supported internally.
 *  - Test 
 */

/* handle to a Redis server connection */
typedef struct _cr_redis* REDIS;

#define CREDIS_OK 0
#define CREDIS_ERR -90
#define CREDIS_ERR_NOMEM -91
#define CREDIS_ERR_RESOLVE -92
#define CREDIS_ERR_CONNECT -93
#define CREDIS_ERR_SEND -94
#define CREDIS_ERR_RECV -95
#define CREDIS_ERR_TIMEOUT -96
#define CREDIS_ERR_PROTOCOL -97

#define CREDIS_TYPE_NONE 1
#define CREDIS_TYPE_STRING 2
#define CREDIS_TYPE_LIST 3
#define CREDIS_TYPE_SET 4

#define CREDIS_SERVER_MASTER 1
#define CREDIS_SERVER_SLAVE 2

typedef enum _cr_aggregate {
  NONE,
  SUM, 
  MIN,
  MAX
} REDIS_AGGREGATE;

#define CREDIS_VERSION_STRING_SIZE 32
#define CREDIS_MULTIPLEXING_API_SIZE 16
#define CREDIS_USED_MEMORY_HUMAN_SIZE 32

typedef struct _cr_info {
  char redis_version[CREDIS_VERSION_STRING_SIZE];
  int arch_bits;
  char multiplexing_api[CREDIS_MULTIPLEXING_API_SIZE];
  long process_id;
  long uptime_in_seconds;
  long uptime_in_days;
  int connected_clients;
  int connected_slaves;
  int blocked_clients;
  unsigned long used_memory;
  char used_memory_human[CREDIS_USED_MEMORY_HUMAN_SIZE];
  long long changes_since_last_save;
  int bgsave_in_progress;
  long last_save_time;
  int bgrewriteaof_in_progress;
  long long total_connections_received;
  long long total_commands_processed;
  long long expired_keys;
  unsigned long hash_max_zipmap_entries;
  unsigned long hash_max_zipmap_value;
  long pubsub_channels;
  unsigned int pubsub_patterns;
  int vm_enabled;
  int role;
} REDIS_INFO;


/*
 * Connection handling
 */

/* `host' is the host to connect to, either as an host name or a IP address, 
 * if set to NULL connection is made to "localhost". `port' is the TCP port 
 * that Redis is listening to, set to 0 will use default port (6379). 
 * `timeout' is the time in milliseconds to use as timeout, when connecting 
 * to a Redis server and waiting for reply, it can be changed after a
 * connection has been made using credis_settimeout() */
REDIS credis_connect(const char *host, int port, int timeout);
int credis_reconnect(REDIS rhnd);

/* set Redis server reply `timeout' in millisecs */ 
void credis_settimeout(REDIS rhnd, int timeout);

void credis_close(REDIS rhnd);

void credis_quit(REDIS rhnd);

int credis_auth(REDIS rhnd, const char *password);

int credis_ping(REDIS rhnd);

/* if a function call returns error it is _possible_ that the Redis server
 * replied with an error message. It is returned by this function. */
char* credis_errorreply(REDIS rhnd);

/* 
 * Commands operating on all the kind of values
 */

/* returns -1 if the key doesn't exists and 0 if it does */
int credis_exists(REDIS rhnd, const char *key);

/* returns -1 if the key doesn't exists and 0 if it was removed 
 * TODO add support to (Redis >= 1.1) remove multiple keys 
 */
int credis_del(REDIS rhnd, const char *key);
int credis_hdel(REDIS rhnd, const char *hash, const char *key);

/* returns type, refer to CREDIS_TYPE_* defines */
int credis_type(REDIS rhnd, const char *key);

/* returns number of keys returned in vector `keyv' */
int credis_keys(REDIS rhnd, const char *pattern, char ***keyv);
#define credis_raw_append cr_appendstrf
int cr_appendstrf(cr_buffer *buf, const char *format, ...);


int credis_randomkey(REDIS rhnd, char **key);

int credis_rename(REDIS rhnd, const char *key, const char *new_key_name);

/* returns -1 if the key already exists */
int credis_renamenx(REDIS rhnd, const char *key, const char *new_key_name);

/* returns size of db */
int credis_dbsize(REDIS rhnd);

/* returns -1 if the timeout was not set; either due to key already has 
   an associated timeout or key does not exist */
int credis_expire(REDIS rhnd, const char *key, int secs);

/* returns time to live seconds or -1 if key does not exists or does not 
 * have expire set */
int credis_ttl(REDIS rhnd, const char *key);

int credis_select(REDIS rhnd, int index);

/* returns -1 if the key was not moved; already present at target 
 * or not found on current db */
int credis_move(REDIS rhnd, const char *key, int index);

int credis_flushdb(REDIS rhnd);

int credis_flushall(REDIS rhnd);


/* 
 * Commands operating on string values 
 */

int credis_set(REDIS rhnd, const char *key, const char *val);
int credis_hset(REDIS rhnd, const char *hash, const char *key, const char *val);

/* returns -1 if the key doesn't exists */
int credis_get(REDIS rhnd, const char *key, char **val);
int credis_hget(REDIS rhnd, const char *hash, const char *key, char **val);

/* returns -1 if the key doesn't exists */
int credis_getset(REDIS rhnd, const char *key, const char *set_val, char **get_val);

/* returns number of values returned in vector `valv'. `keyc' is the number of
 * keys stored in `keyv'. */
int credis_mget(REDIS rhnd, int keyc, const char **keyv, char ***valv);

/* returns -1 if the key already exists and hence not set */
int credis_setnx(REDIS rhnd, const char *key, const char *val);

int credis_raw_sendandreceive(REDIS rhnd, char type);

/* TODO
 * SETEX key time value Set+Expire combo command
 * MSET key1 value1 key2 value2 ... keyN valueN set a multiple keys to multiple values in a single atomic operation
 * MSETNX key1 value1 key2 value2 ... keyN valueN set a multiple keys to multiple values in a single atomic operation if none of
 */

/* if `new_val' is not NULL it will return the value after the increment was performed */
int credis_incr(REDIS rhnd, const char *key, int *new_val);

/* if `new_val' is not NULL it will return the value after the increment was performed */
int credis_incrby(REDIS rhnd, const char *key, int incr_val, int *new_val);

/* if `new_val' is not NULL it will return the value after the decrement was performed */
int credis_decr(REDIS rhnd, const char *key, int *new_val);

/* if `new_val' is not NULL it will return the value after the decrement was performed */
int credis_decrby(REDIS rhnd, const char *key, int decr_val, int *new_val);

/* returns new length of string after `val' has been appended */
int credis_append(REDIS rhnd, const char *key, const char *val);

int credis_substr(REDIS rhnd, const char *key, int start, int end, char **substr);


/*
 * Commands operating on lists 
 */

int credis_rpush(REDIS rhnd, const char *key, const char *element);

int credis_lpush(REDIS rhnd, const char *key, const char *element);

/* returns length of list */
int credis_llen(REDIS rhnd, const char *key);

/* returns number of elements returned in vector `elementv' */
int credis_lrange(REDIS rhnd, const char *key, int start, int range, char ***elementv);

int credis_ltrim(REDIS rhnd, const char *key, int start, int end);

/* returns -1 if the key doesn't exists */
int credis_lindex(REDIS rhnd, const char *key, int index, char **element);

int credis_lset(REDIS rhnd, const char *key, int index, const char *element);

/* returns number of elements removed */
int credis_lrem(REDIS rhnd, const char *key, int count, const char *element);

/* returns -1 if the key doesn't exists */
int credis_lpop(REDIS rhnd, const char *key, char **val);
int credis_blpop(REDIS rhnd, const char *key, const char *seconds, char **val);

/* returns -1 if the key doesn't exists */
int credis_rpop(REDIS rhnd, const char *key, char **val);
int credis_brpop(REDIS rhnd, const char *key, const char *seconds, char **val);

/* TODO 
 * BLPOP key1 key2 ... keyN timeout Blocking LPOP
 * BRPOP key1 key2 ... keyN timeout Blocking RPOP
 * RPOPLPUSH srckey dstkey Return and remove (atomically) the last element of the source List stored at _srckey_ and push the same element to the destination List stored at _dstkey_ 
 */


/*
 * Commands operating on sets 
 */

/* returns -1 if the given member was already a member of the set */
int credis_sadd(REDIS rhnd, const char *key, const char *member);

/* returns -1 if the given member is not a member of the set */
int credis_srem(REDIS rhnd, const char *key, const char *member);

/* returns -1 if the given key doesn't exists else value is returned in `member' */
int credis_spop(REDIS rhnd, const char *key, char **member);

/* returns -1 if the member doesn't exists in the source set */
int credis_smove(REDIS rhnd, const char *sourcekey, const char *destkey, 
                 const char *member);

/* returns cardinality (number of members) or 0 if the given key doesn't exists */
int credis_scard(REDIS rhnd, const char *key);

/* returns -1 if the key doesn't exists and 0 if it does */
int credis_sismember(REDIS rhnd, const char *key, const char *member);

/* returns number of members returned in vector `members'. `keyc' is the number of
 * keys stored in `keyv'. */
int credis_sinter(REDIS rhnd, int keyc, const char **keyv, char ***members);

/* `keyc' is the number of keys stored in `keyv' */
int credis_sinterstore(REDIS rhnd, const char *destkey, int keyc, const char **keyv);

/* returns number of members returned in vector `members'. `keyc' is the number of
 * keys stored in `keyv'. */
int credis_sunion(REDIS rhnd, int keyc, const char **keyv, char ***members);

/* `keyc' is the number of keys stored in `keyv' */
int credis_sunionstore(REDIS rhnd, const char *destkey, int keyc, const char **keyv);

/* returns number of members returned in vector `members'. `keyc' is the number of
 * keys stored in `keyv'. */
int credis_sdiff(REDIS rhnd, int keyc, const char **keyv, char ***members);

/* `keyc' is the number of keys stored in `keyv' */
int credis_sdiffstore(REDIS rhnd, const char *destkey, int keyc, const char **keyv);

/* returns number of members returned in vector `members' */
int credis_smembers(REDIS rhnd, const char *key, char ***members);

/* TODO Redis >= 1.1
 * SRANDMEMBER key Return a random member of the Set value at key
 */


/* 
 * Commands operating on sorted sets
 */

/* returns -1 if member was already a member of the sorted set and only score was updated, 
 * 0 is returned if the new element was added */
int credis_zadd(REDIS rhnd, const char *key, double score, const char *member);

/* returns -1 if the member was not a member of the sorted set */
int credis_zrem(REDIS rhnd, const char *key, const char *member);

/* returns -1 if the member was not a member of the sorted set, the score of the member after
 * the increment by `incr_score' is returned by `new_score' */
int credis_zincrby(REDIS rhnd, const char *key, double incr_score, const char *member, double *new_score);

/* returns the rank of the given member or -1 if the member was not a member of the sorted set */
int credis_zrank(REDIS rhnd, const char *key, const char *member);

/* returns the reverse rank of the given member or -1 if the member was not a member of the sorted set */
int credis_zrevrank(REDIS rhnd, const char *key, const char *member);

/* returns number of elements returned in vector `elementv' 
 * TODO add support for WITHSCORES */
int credis_zrange(REDIS rhnd, const char *key, int start, int end, char ***elementv);

/* returns number of elements returned in vector `elementv' 
 * TODO add support for WITHSCORES */
int credis_zrevrange(REDIS rhnd, const char *key, int start, int end, char ***elementv);

/* returns cardinality or -1 if `key' does not exist */
int credis_zcard(REDIS rhnd, const char *key);

/* returns -1 if the `key' does not exist or the `member' is not in the sorted set,
 * score is returned in `score' */
int credis_zscore(REDIS rhnd, const char *key, const char *member, double *score);

/* returns number of elements removed or -1 if key does not exist */
int credis_zremrangebyscore(REDIS rhnd, const char *key, double min, double max);

/* returns number of elements removed or -1 if key does not exist */
int credis_zremrangebyrank(REDIS rhnd, const char *key, int start, int end);

/* TODO
 * ZRANGEBYSCORE key min max Return all the elements with score >= min and score <= max (a range query) from the sorted set
 */

/* `keyc' is the number of keys stored in `keyv'. `weightv' is optional, if not 
 * NULL, `keyc' is also the number of weights stored in `weightv'. */
int credis_zinterstore(REDIS rhnd, const char *destkey, int keyc, const char **keyv, 
                       const int *weightv, REDIS_AGGREGATE aggregate);

/* `keyc' is the number of keys stored in `keyv'. `weightv' is optional, if not 
 * NULL, `keyc' is also the number of weights stored in `weightv'. */
int credis_zunionstore(REDIS rhnd, const char *destkey, int keyc, const char **keyv, 
                       const int *weightv, REDIS_AGGREGATE aggregate);

/* 
 * Commands operating on hashes
 */

/* TODO
 * HMSET key field1 value1 ... fieldN valueN Set the hash fields to their respective values.
 * HINCRBY key field integer Increment the integer value of the hash at _key_ on _field_ with _integer_.
 * HEXISTS key field Test for existence of a specified field in a hash
 * HLEN key Return the number of items in a hash.
 * HKEYS key Return all the fields in a hash.
 * HVALS key Return all the values in a hash.
 * HGETALL key Return all the fields and associated values in a hash.
 */


/*
 * Sorting 
 */

/* returns number of elements returned in vector `elementv' */
int credis_sort(REDIS rhnd, const char *query, char ***elementv);


/*
 * Transactions
 */

/* TODO
 * MULTI/EXEC/DISCARD Redis atomic transactions
 */


/*
 * Publish/Subscribe
 */

/* TODO
 * SUBSCRIBE/UNSUBSCRIBE/PUBLISH Redis Public/Subscribe messaging paradigm implementation
 */


/* 
 * Persistence control commands 
 */

int credis_save(REDIS rhnd);

int credis_bgsave(REDIS rhnd);

/* returns UNIX time stamp of last successfull save to disk */
int credis_lastsave(REDIS rhnd);

int credis_shutdown(REDIS rhnd);

int credis_bgrewriteaof(REDIS rhnd);


/*
 * Remote server control commands 
 */

/* Because the information returned by the Redis changes with virtually every 
 * major release, credis tries to parse for as many fields as it is aware of, 
 * staying backwards (and forwards) compatible with older (and newer) versions 
 * of Redis. 
 * Information fields not supported by the Redis server connected to, are set
 * to zero. */
int credis_info(REDIS rhnd, REDIS_INFO *info);

int credis_monitor(REDIS rhnd);

/* setting host to NULL and/or port to 0 will turn off replication */
int credis_slaveof(REDIS rhnd, const char *host, int port);

/* TODO
 * CONFIG Configure a Redis server at runtime
 */


#ifdef __cplusplus
}
#endif

#endif /* __CREDIS_H */
