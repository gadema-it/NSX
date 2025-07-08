#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <memory.h>
#include <vector>

template <typename T>
struct MemoryPool
{
    int bucket_size;
    int bucket_count;
    int item_count;
    std::vector<T*> free_items;
    std::vector<T**> buckets;

    MemoryPool(int bucket_size, int initial_size = 0):
        bucket_size(bucket_size),
        bucket_count(initial_size) {}


    ~MemoryPool() {
        for(auto b: buckets) {
            std::free(b);
        }
    }


    T* allocate() { //TODO allocN, reserveN?
        if (free_items.empty()) {
            addBucket(); //TODO Fix add bucket
        }
        T* free_item = free_items.back();
        free_items.pop_back();
        return new(free_item)T();
        //return free_item;
    }


    void deallocate(T* item) {
        item->~T();
        free_items.push_back(item);
    }


    std::vector<T*> allocateN(int n) {
        if (n > free_items.size()) {
            int bucket_needed = bucket_size / n;
            for (int i = bucket_needed; i > 0; i--) { // push back from n to (n - free_old)
                addBucket();
            }
        }
        std::vector<T*> new_items;
        for (int i = free_items.size() - 1; i > free_items.size() - n - 1; i--) {
            new_items.push_back(new (free_items[i])T());
        }
        free_items.resize(free_items.size() - n);
        return new_items;
    }


    void addBucket() {
        T* bucket = static_cast<T*>(std::malloc(sizeof(T) * bucket_size));
        buckets.push_back(&bucket);
        for (int i = 0; i < bucket_size; i++) {
            free_items.push_back(bucket + i);
        }
    }


    void removeBucket() {

    }


//    T* findItem(int index) {
//        T* item = nullptr;
//        for (int i = 0; i < bucket_count; i++) {
//            if (index > i * bucket_size) continue;
//            for (int j = 0; j < bucket_size; j++)
//            {

//            }
//        }
//        return item;
//    }


    void compactMemory() {

    }

};

#endif // MEMORYPOOL_H
