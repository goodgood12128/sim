import numpy as np

# 行数
num_rows = 99999
# 起始时间
start_time = 0
# 时间间隔的均值和标准差
time_mean = 540000
time_std = 100
# 初始lsn
start_lsn = 0
# 大小的均值和标准差
size_mean = 192
size_std = 10

# 文件名
filename = 'trace_size'+str(size_mean)+'_time'+str(time_mean)

# 生成数据
time_values = (np.round(np.random.normal(loc=time_mean, scale=time_std, size=num_rows))).astype(int)
size_values = (np.round(np.abs(np.random.normal(loc=size_mean, scale=size_std, size=num_rows)) / 8) * 8).astype(int)
device_values = np.zeros(num_rows, dtype=int)
operation_values = np.random.choice([0, 1], size=num_rows)

# 写入文件
with open(filename, 'w') as file:
    for i in range(num_rows):
        line = f"{start_time + time_values[i]} {device_values[i]} {start_lsn} {size_values[i]} 1\n"
        file.write(line)
        start_time += time_values[i]
        start_lsn += size_values[i]

print(f"文件已生成：{filename}")
