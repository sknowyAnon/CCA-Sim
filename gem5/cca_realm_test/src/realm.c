#include "../include/realm.h"
#include "../include/realm.h"

// 内存加密函数（支持64位地址）
void encrypt_memory_64(uint64_t base_address, size_t size, uint8_t* key, size_t key_length) {
    if (base_address == 0 || size == 0 || key == NULL || key_length == 0) {
        return;
    }
    
    // 将64位地址转换为指针
    uint8_t* mem = (uint8_t*)(uintptr_t)base_address;
    
    // 确保我们有访问权限（实际应用中需要更完善的检查）
    if (mem == NULL) {
        fprintf(stderr, "Invalid memory address: 0x%016" PRIx64 "\n", base_address);
        return;
    }
    
    for (size_t i = 0; i < size; i++) {
        mem[i] ^= key[i % key_length];  // 使用异或操作进行加密
    }
}


// 生成随机密钥
uint8_t* generate_random_key(size_t key_length) {
    if (key_length == 0) {
        return NULL;
    }
    
    uint8_t* key = (uint8_t*)malloc(key_length);
    if (key == NULL) {
        return NULL;
    }
    
    // 使用更安全的随机数生成（如果可用）
    for (size_t i = 0; i < key_length; i++) {
        key[i] = rand() % 256;
    }
    
    return key;
}

// 打印内存内容（调试用）
void print_memory_64(uint64_t base_address, size_t size) {
    uint8_t* mem = (uint8_t*)(uintptr_t)base_address;
    printf("        Memory content at 0x%016" PRIx64 ": ", base_address);
    for (size_t i = 0; i < (size > 16 ? 16 : size); i++) { // 限制打印长度
        printf("%02X ", mem[i]);
    }
    if (size > 16) printf("...");
    printf("\n");
}

// 创建共享内存
void* create_shared_memory(realm_memory_t *mem, const char *name, size_t size) {
    snprintf(mem->shm_name, sizeof(mem->shm_name), "/realm_shm_%s", name);
    mem->shm_fd = shm_open(mem->shm_name, O_CREAT | O_RDWR, 0666); // 使用shm_open()创建 打开read write共享内存对象
    if (mem->shm_fd == -1) return NULL;

    // 使用ftruncate()设置共享内存大小
    if (ftruncate(mem->shm_fd, size) == -1) {
        close(mem->shm_fd);
        shm_unlink(mem->shm_name);
        return NULL;
    }

    // 使用mmap()将共享内存映射到进程地址空间
    mem->base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mem->shm_fd, 0);
    if (mem->base == MAP_FAILED) {
        close(mem->shm_fd);
        shm_unlink(mem->shm_name);
        return NULL;
    }

    mem->size = size;
    mem->is_shared = true;
    return mem->base;
}

// 销毁共享内存
void destroy_shared_memory(realm_memory_t *mem) {
    if (mem->base) munmap(mem->base, mem->size);
    if (mem->shm_fd != -1) close(mem->shm_fd);
    shm_unlink(mem->shm_name);
}

// 创建Realm
realm_id_t realm_create(realm_monitor_t *monitor, const char *program, 
                       char *const argv[], size_t private_mem_size, 
                       size_t shared_mem_size, bool is_malicious) {
    realm_context_t *new_realm = malloc(sizeof(realm_context_t)); // 分配并初始化realm_context_t结构
    if (!new_realm) return 0;

    static realm_id_t next_id = 1;
    new_realm->id = next_id++; // 分配唯一ID（静态变量next_id递增）
    new_realm->program = strdup(program);
    new_realm->argv = malloc(sizeof(char*) * 32); // 复制程序路径和参数（深拷贝）
    new_realm->is_malicious = is_malicious; // 设置恶意标志

    int i = 0;
    if (argv) {
        while (argv[i] && i < 31) {
            new_realm->argv[i] = strdup(argv[i]);
            i++;
        }
    }
    new_realm->argv[i] = NULL;

    // 私有内存（使用匿名映射，确保隔离）
    new_realm->private_memory.base = mmap(NULL, private_mem_size,
                                         PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_realm->private_memory.base == MAP_FAILED) {
        perror("mmap private");
        free(new_realm);
        return 0;
    }
    new_realm->private_memory.size = private_mem_size;
    printf("[REALM] Realm %lu Private Mem Base: 0x%lx, Size: 0x%lx\n", new_realm->id, new_realm->private_memory.base,new_realm->private_memory.size);
    new_realm->private_memory.is_shared = false;
    new_realm->private_memory.shm_fd = -1;

    // 初始化一些私有数据
    sprintf(new_realm->private_memory.base, "Realm %lu private data", new_realm->id);

    new_realm->key_length=32;

    new_realm->key=generate_random_key(new_realm->key_length);
    encrypt_memory_64(new_realm->private_memory.base,new_realm->private_memory.size,new_realm->key,new_realm->key_length);

    printf("[REALM] Realm %lu encrypted memory:\n", new_realm->id);
    print_memory_64(new_realm->private_memory.base,new_realm->private_memory.size);

    // 创建共享内存
    if (shared_mem_size > 0) {
        char shm_name[32];
        snprintf(shm_name, sizeof(shm_name), "realm%lu_shared", new_realm->id);
        if (!create_shared_memory(&new_realm->shared_memory, shm_name, shared_mem_size)) {
            munmap(new_realm->private_memory.base, new_realm->private_memory.size);
            free(new_realm);
            return 0;
        }
        sprintf(new_realm->shared_memory.base, "Realm %lu shared data", new_realm->id);
    } else {
        memset(&new_realm->shared_memory, 0, sizeof(new_realm->shared_memory));
    }

    new_realm->pid = -1;

    monitor->realms = realloc(monitor->realms, 
                             (monitor->realm_count + 1) * sizeof(realm_context_t));
    monitor->realms[monitor->realm_count++] = *new_realm;
    free(new_realm);

    return next_id - 1;
}

// 恶意行为：尝试访问其他Realm的内存
void attempt_malicious_access(realm_monitor_t *monitor, realm_id_t attacker_id) {
    for (int i = 0; i < monitor->realm_count; i++) {
        if (monitor->realms[i].id == attacker_id && monitor->realms[i].is_malicious) {
            realm_context_t *attacker = &monitor->realms[i];
            
            // 查找目标Realm（非自身）
            for (int j = 0; j < monitor->realm_count; j++) {
                if (monitor->realms[j].id != attacker_id) {
                    realm_context_t *target = &monitor->realms[j];
                    
                    printf("\n[REALM] Realm %lu (malicious) attempting to access Realm %lu private memory...\n",
                           attacker_id, target->id);
                    
                    // 尝试读取目标私有内存
                    char buffer[256];
                    memcpy(buffer, target->private_memory.base, 
                           sizeof(buffer) < target->private_memory.size ? 
                           sizeof(buffer) : target->private_memory.size);
                    //sleep(0.5);
                    // 检查是否成功访问（在实际隔离中应该失败）
                    if (strstr(buffer, "private data") != NULL) {
                        printf("[REALM] ATTACK SUCCESSFUL! Realm %lu accessed Realm %lu private data: %s\n",
                               attacker_id, target->id, buffer);
                    } else {
                        printf("[REALM] ATTACK FAILED! Realm %lu cannot access Realm %lu private memory\n",
                               attacker_id, target->id);
                    }
                    
                    // 尝试访问共享内存（应该成功）
                    if (target->shared_memory.base) {
                        printf("\n[REALM] Realm %lu attempting to access Realm %lu shared memory...\n",
                               attacker_id, target->id);
                        
                        memcpy(buffer, target->shared_memory.base, 
                               sizeof(buffer) < target->shared_memory.size ? 
                               sizeof(buffer) : target->shared_memory.size);
                        
                        if (strstr(buffer, "shared data") != NULL) {
                            printf("[REALM] Access to shared memory successful (expected): %s\n", buffer);
                        } else {
                            printf("[REALM] Shared memory access failed unexpectedly\n");
                        }
                    }
                    
                    return;
                }
            }
        }
    }
}

// 启动Realm进程
bool realm_start(realm_monitor_t *monitor, realm_id_t id) {
    for (int i = 0; i < monitor->realm_count; i++) {
        if (monitor->realms[i].id == id) {
            realm_context_t *realm = &monitor->realms[i];
            
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                return false;
            }
            
            if (pid == 0) { // 子进程
                printf("[REALM] Realm %lu (%s) starting program: %s\n", 
                       id, realm->is_malicious ? "malicious" : "normal", realm->program);
                //system(realm->program);
                // 在真实系统中，这里会有更严格的内存隔离
                
                // 如果是恶意Realm，尝试攻击
                if (realm->is_malicious) {
                    sleep(1); // 等待其他Realm启动
                    attempt_malicious_access(monitor, id);
                    exit(0);
                }
                
                // 正常Realm的行为
                printf("[REALM] Realm %lu running normally\n", id);
                
                // 演示正常的内存访问
                printf("[REALM] Realm %lu decrypting memory...\n",realm->id);
                encrypt_memory_64(realm->private_memory.base, realm->private_memory.size, realm->key, realm->key_length);
                printf("[REALM] Realm %lu accessing its own private data: %s\n",
                       id, (char*)realm->private_memory.base);
                encrypt_memory_64(realm->private_memory.base, realm->private_memory.size, realm->key, realm->key_length);
                
                if (realm->shared_memory.base) {
                    printf("[REALM] Realm %lu accessing its own shared data: %s\n",
                           id, (char*)realm->shared_memory.base);
                }
                
                sleep(10); // 保持运行一段时间
                exit(0);
            } else { // 父进程
                realm->pid = pid;
                printf("[REALM] Realm %lu started with PID %d\n", id, pid);
                //system(realm->program);
                return true;
            }
        }
    }
    return false;
}

// 停止Realm
bool realm_stop(realm_monitor_t *monitor, realm_id_t id) {
    for (int i = 0; i < monitor->realm_count; i++) {
        if (monitor->realms[i].id == id && monitor->realms[i].pid != -1) {
            kill(monitor->realms[i].pid, SIGTERM);
            int status;
            waitpid(monitor->realms[i].pid, &status, 0);
            monitor->realms[i].pid = -1;
            return true;
        }
    }
    return false;
}

// 销毁Realm
void realm_destroy(realm_monitor_t *monitor, realm_id_t id) {
    for (int i = 0; i < monitor->realm_count; i++) {
        if (monitor->realms[i].id == id) {
            if (monitor->realms[i].pid != -1) {
                realm_stop(monitor, id);
            }
            
            munmap(monitor->realms[i].private_memory.base, 
                  monitor->realms[i].private_memory.size);
            
            if (monitor->realms[i].shared_memory.base) {
                destroy_shared_memory(&monitor->realms[i].shared_memory);
            }
            
            free(monitor->realms[i].program);
            for (int j = 0; monitor->realms[i].argv[j]; j++) {
                free(monitor->realms[i].argv[j]);
            }
            free(monitor->realms[i].argv);
            
            memmove(&monitor->realms[i], &monitor->realms[i+1], 
                   (monitor->realm_count - i - 1) * sizeof(realm_context_t));
            monitor->realm_count--;
            monitor->realms = realloc(monitor->realms, 
                                    monitor->realm_count * sizeof(realm_context_t));
            return;
        }
    }
}
