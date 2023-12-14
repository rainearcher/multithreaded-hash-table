# Hash Hash Hash
This project compares three implementations of a hash table. Base is implemented serially. V1 is implemented with a single mutex, and performs slower. V2 is implemented with multiple mutexes and performs the fastest.

## Building
```shell
make
```
This generates the object files and the hash-table-tester executable.

## Running
To run the executable:
```shell
./hash-table-tester -t 4 -s 50000
```
-t is the number of threads 
-s is the number of hash table entries per thread

results should look like:
```
Hash table base: 215,934 usec
  - 0 missing
Hash table v1: 478,153 usec
  - 0 missing
Hash table v2: 63,968 usec
  - 0 missing
```

## First Implementation
I added a mutex to the hash_table struct. In the `hash_table_v1_add_entry` function, I lock the mutex at the beginning of the function, and I unlock the mutex at the end of the function. So adding entries to the table is atomic. Locking the table is reasonable because we are looking for the easiest and clearest form of atomicity.

### Performance
```shell
./hash-table-tester -t 4 -s 50000
```
-t is the number of threads 
-s is the number of hash table entries per thread

results:
```
Hash table base: 215,934 usec
  - 0 missing
Hash table v1: 478,153 usec
  - 0 missing
Hash table v2: 63,968 usec
  - 0 missing
```
Version 1 is a little slower/faster than the base version the because there is extra overhead in defining the mutex for the hash_table, then locking and unlocking the mutex when adding entries.

## Second Implementation
I added a mutex to the hash_table_entry struct. In the `hash_table_v2_add_entry` function, I first get the hash_table_entry, then I lock the mutex. This is because its possible for another thread to change the list_head while the current thread attempts to retrieve it.

Then I check if the list_entry already exists: if so, I change its value and unlock the mutex before returning. This frees the hash_table_entry for other threads to change it.

If the list_entry does not exist, I unlock the mutex. This frees hash_table_entry for other threads to modify while I allocate memory for the list_entry, then update its key and value.

I then lock the hash_table_entry, so that I can update the list_head atomically with the new list_entry. I then unlock the mutex and return from the function.

### Performance
```shell
./hash-table-tester -t 4 -s 50000
```
-t is the number of threads 
-s is the number of hash table entries per thread

results:
```
Hash table base: 215,934 usec
  - 0 missing
Hash table v1: 478,153 usec
  - 0 missing
Hash table v2: 63,968 usec
  - 0 missing
```

V2 is much faster than V1 and the base because it locks the hash_table entries instead of the entire table. This means each thread can add list entries to separate table entries concurrently. Additionally, each thread unlocks the current entry when it is busy allocating memory for a list entry, so this allows other threads to modify it while the current thread is performing a time-consuming operation. 

## Cleaning up
```shell
make clean
```
This cleans up all generated files (executable, and object files)