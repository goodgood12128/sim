#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>
#include <time.h>

// 哈希表节点
typedef struct HashNode {
    long long key;
    struct HashNode* next;
} HashNode;

// 哈希表
typedef struct HashTable {
    size_t size;
    HashNode** table;
} HashTable;

// 哈希函数
long long hash(int key, size_t size) {
    return key % size;
}

// 初始化哈希表
HashTable* initHashTable(size_t size) {
    HashTable* ht = malloc(sizeof(HashTable));
    ht->size = size;
    ht->table = calloc(size, sizeof(HashNode*));
    return ht;
}

// 插入键值对到哈希表
void insert(HashTable* ht, int key) {
    long long index = hash(key, ht->size);
    
    HashNode* newNode = malloc(sizeof(HashNode));
    newNode->key = key;
    newNode->next = ht->table[index];
    ht->table[index] = newNode;
}

// 查询键是否存在于哈希表
bool contains(HashTable* ht, long long int key) {
    long long index = hash(key, ht->size);

    HashNode* current = ht->table[index];
    while (current != NULL) {
        if (current->key == key) {
            return true;
        }
        current = current->next;
    }

    return false;
}

// 释放哈希表的内存
void freeHashTable(HashTable* ht) {
    for (size_t i = 0; i < ht->size; i++) {
        HashNode* current = ht->table[i];
        while (current != NULL) {
            HashNode* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(ht->table);
    free(ht);
}

// 比较函数用于qsort
int compare(const void *a, const void *b) {
    return (*(long long int *)a - *(long long int *)b);
}

// 计算百分位数
void calculatePercentiles(long long int *intervals, size_t count) {
    // 排序
    qsort(intervals, count, sizeof(long long int), compare);

    // 输出百分位数
    for (double percentile = 25; percentile <= 100; percentile += 25) {
        size_t index = (size_t)((percentile / 100.0) * count-1);
        printf("%lu ", index);
        printf("%lld ", intervals[index]);
    }
    printf("\n");
}

void processFile(const char *filename);


void processFile(const char *filename) {
    char fullpath[256];  // Assuming maximum path length is 255 characters
    snprintf(fullpath, sizeof(fullpath), "%s/%s", "/home/zmr/sim/4_data/16GB/public", filename);
    FILE *file = fopen(fullpath, "r");

    printf("%s ", filename);
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    int writeRequests = 0;
    int updateRequests = 0;
    int readRequests = 0;
    int readAvgSize = 0;
    int writeFirstAvgSize = 0;
    int updateAvgSize = 0;
    int writeAvgSize=0;
    double intervalTime=0;
    HashTable* writeAddresses = initHashTable(12000000);

    int requestType;
    int operationSize;
    long long int logicalAddress;
    long long int *intervals = NULL;
    size_t count = 0;
    long long int prev_time=0, current_time=0;

    char line[256];
    long long int prevTime = 0;
    long long int maxInterval =0;
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%lld %*d %lld %d %d", &current_time, &logicalAddress, &operationSize, &requestType);
        if (count > 0) {
            long long int interval = current_time - prev_time;
            if(maxInterval < interval) maxInterval = interval;
            intervals = realloc(intervals, (count + 1) * sizeof(long long int));
            intervals[count-1] = interval;
        }
        prev_time = current_time;
        count++;
        // printf(" %lld %d %d\n", logicalAddress, operationSize, requestType);
        if(requestType == 1){
            readRequests++;
            readAvgSize += operationSize;
        }else if(contains(writeAddresses, logicalAddress)){
            updateRequests++;
            updateAvgSize += operationSize;
            writeAvgSize += operationSize;
        }else{
            writeRequests++;
            writeFirstAvgSize += operationSize;
            writeAvgSize += operationSize;
            insert(writeAddresses, logicalAddress);  // 插入写请求地址到哈希表
        }
    }
    // printf("max %lld ", maxInterval);
    
    freeHashTable(writeAddresses);

    // fclose(file);
    // qsort(intervals, count, sizeof(long long int), compare);
    //  if (count > 0) {
    //     calculatePercentiles(intervals, count);
    //     free(intervals);  // 释放动态分配的内存
    // }

    // printf("文件: %s\n", filename);
    // printf("第一次到达的写请求数目: %d\n", writeRequests);
    // printf("更新请求数目: %d\n", updateRequests);
    // printf("读请求数目: %d\n", readRequests);
    printf("%s %d %d %d\n", filename, writeRequests, updateRequests, readRequests);
    // printf("%s ", filename);
    // if(readRequests != 0) printf("%f ", 0.5*readAvgSize/readRequests); else printf("%d ", 0);
    // if(updateRequests != 0) printf("%f ", 0.5*updateAvgSize/updateRequests); else printf("%d ", 0);
    // if(writeRequests != 0) printf("%f ", 0.5*writeFirstAvgSize/writeRequests); else printf("%d ", 0);
    // if((writeRequests+updateRequests) != 0) printf("%f\n", 0.5*writeAvgSize/(writeRequests+updateRequests)); else printf("%d\n", 0);
    }


double generate_normal_random(double mean, double variance) {
    // 使用Box-Muller变换生成标准正态分布的随机数
    double u1, u2, z0;
    do {
        u1 = ((double)rand() / RAND_MAX) * 2 - 1;
        u2 = ((double)rand() / RAND_MAX) * 2 - 1;
        z0 = u1 * u1 + u2 * u2;
    } while (z0 > 1 || z0 == 0);

    double z1 = sqrt(-2.0 * log(z0) / z0);
    
    // Apply range restrictions
    double random_number = mean + sqrt(variance) * u1 * z1;
    return (random_number < 0) ? 0 : ((random_number > 1) ? 1 : random_number);
}

// int main() {
//     double num = 3.34;
//     double int_part, frac_part;

//     // 使用 modf 函数将浮点数拆分为整数部分和小数部分
//     frac_part = modf(num/3, &int_part);

//     // 输出整数部分和小数部分
//     printf("Original number: %f\n", num);
//     printf("Integer part: %f\n", int_part);
//     printf("Fractional part: %f %d %d\n", frac_part, (frac_part==0)? (int)num:(int)num+1);

//     return 0;
// }

int main(){
    int PAGE_INDEX=3,plane_die=3,i,j;
    int mutli_subs[100],subs[100];
    for(int i = 0; i<100; i++){
        subs[i] = i;
    }
    for(i=0;i<plane_die;i++){
        for(j=0;j<PAGE_INDEX;j++){
            mutli_subs[i*PAGE_INDEX+j]=subs[j*plane_die+i];
        }
    }
    for(i = 0; i<plane_die*PAGE_INDEX; i++){
        printf(" %d ", mutli_subs[i]);
    }
    return 0;
}