#!/bin/bash

# 提取文件中包含 "sub_req_size:" 的行，并提取数字
numbers=$(grep "sub_req_size:" exchange_statistic.dat | sed 's/sub_req_size://')

# 计算平均值
sum=0
count=0
for number in $numbers; do
  sum=$((sum + number))
  count=$((count + 1))
done

average=$((sum / count))

echo "平均值: $average"

