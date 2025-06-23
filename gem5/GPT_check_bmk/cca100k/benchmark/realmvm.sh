#!/bin/bash
# 定义要并行运行的命令数组
# commands=(
#   "./realm1"
#   "./realm2"
#   "./realm3"
# )

# # 启动所有命令
# pids=()
# for cmd in "${commands[@]}"; do
#   echo "[REALM] User application $cmd in Realm."
#   eval "$cmd" &
#   pids+=($!)  # 保存进程ID
# done

# # 等待所有进程完成
# for pid in "${pids[@]}"; do
#   wait "$pid"
#   echo "[REALM] User application $pid finished."
# done

for i in `seq 1 3`
do
{
    echo "[REALM] Realm VM$i is starting..."
    ./benchmark/realm$i;
} &
done

echo "[REALM] All Realm VM finished"