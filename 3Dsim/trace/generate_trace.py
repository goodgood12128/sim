import numpy as np

#生成数据，请求的总大小为16GB，请求的大小分为4KB、8KB、16KB、32KB、64KB，请求的间隔分为9us、90us、900us、9000us和90000us，起始地址顺序or随机

# 行数
num_rows = 900000

# 初始lsn
start_lsn = 0
# 大小的均值和标准差
size_mean = [8, 16, 32, 64, 128]
size_std = 10

# 起始时间
start_time = 0
# 时间间隔的均值和标准差
time_mean = [9000, 90000, 900000, 9000000, 90000000]
time_std = [10, 100, 1000, 1000, 1000]

r_w_p = [[0.2, 0.8], [0.5, 0.5], [0.8, 0.2]]

# 生成数据
for size_index in range(5):
    for time_index in range(5):
        for index in range(3):
            # 文件名
            filename = 'trace_seq_size'+str(size_mean[size_index])+'_time'+str(time_mean[time_index])+"_p"+str(r_w_p[index])

            time_values = (np.round(np.abs(np.random.normal(loc=time_mean[time_index], scale=time_std[time_index], size=num_rows)))).astype(int)
            size_values = (np.round(np.abs(np.random.normal(loc=size_mean[size_index], scale=size_std, size=num_rows)) / 8) * 8).astype(int)
            device_values = np.zeros(num_rows, dtype=int)
            operation_values = np.random.choice([0, 1], size=num_rows, p=r_w_p[index])

            # 写入文件
            with open(filename, 'w') as file:
                for i in range(num_rows):
                    if size_values[i] == 0:
                        size_values[i] = 4
                    line = f"{start_time + time_values[i]} {device_values[i]} {start_lsn} {size_values[i]} {operation_values[i]}\n"
                    file.write(line)
                    start_time += time_values[i]
                    start_lsn += size_values[i]

            print(f"文件已生成：{filename}")

            # # 文件名
            # filename = 'trace_random_size'+str(size_mean[size_index])+'_time'+str(time_mean[time_index])+"_p"+str(r_w_p[index])

            # time_values = (np.round(np.abs(np.random.normal(loc=time_mean[time_index], scale=time_std[time_index], size=num_rows)))).astype(int)
            # size_values = (np.round(np.abs(np.random.normal(loc=size_mean[size_index], scale=size_std, size=num_rows)) / 8) * 8).astype(int)
            # device_values = np.zeros(num_rows, dtype=int)
            # operation_values = np.random.choice([0, 1], size=num_rows, p=r_w_p[index])

            # # 写入文件
            # with open(filename, 'w') as file:
            #     for i in range(num_rows):
            #         if size_values[i] == 0:
            #             size_values[i] = 4
            #         line = f"{start_time + time_values[i]} {device_values[i]} {start_lsn} {size_values[i]} {operation_values[i]}\n"
            #         file.write(line)
            #         start_time += time_values[i]
            #         start_lsn = np.random.randint(0, num_rows*size_mean[size_index])

            # print(f"文件已生成：{filename}")
