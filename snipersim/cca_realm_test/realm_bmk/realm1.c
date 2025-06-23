#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdint.h>

#define SHM_SIZE 1024

typedef struct {
    int private_data;  // 私有数据
    char shared_msg[100];  // 共享消息
    uint8_t realm_key;
} MemoryBlock;

int main() {
    // 创建私有内存
    MemoryBlock *private_mem = malloc(sizeof(MemoryBlock));
    private_mem->private_data = 100;
    printf("[REALM] Realm1 private data:  %d\n", private_mem->private_data);
    srand(time(NULL));
    private_mem->realm_key = rand() % 256;
    
        // encrypt私有数据
    private_mem->private_data ^= private_mem->realm_key;
    printf("[REALM] Realm1 private data encrypted: %d\n", private_mem->private_data);
    
    // 创建共享内存
    key_t key = ftok("shmfile", 65);
    int shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    
    // 附加到共享内存
    MemoryBlock *shared_mem = (MemoryBlock *)shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    
    // 写入共享内存
    strcpy(shared_mem->shared_msg, "Hello from Realm1!");
    printf("[REALM] Realm1 writing shared memory: %s\n", shared_mem->shared_msg);
    
    // 等待程序2响应
    printf("[REALM] Realm1 waiting for Realm2...\n");
    sleep(2);
    
    // 读取程序2的响应
    printf("[REALM] Realm1 receive message from Realm2: %s\n", shared_mem->shared_msg);
   
    
    // 分离共享内存
    shmdt(shared_mem);
    
    // 删除共享内存(通常由最后一个使用的程序删除)
    // shmctl(shmid, IPC_RMID, NULL);
    
    free(private_mem);
    return 0;
}
