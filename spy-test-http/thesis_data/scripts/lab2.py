import glob
import re
import numpy as np
import statistics

def gen_result_lab_2(dir_name):
    file_paths = glob.glob(dir_name)
    file_paths = sorted(file_paths)

    eps = []
    for i,file_path in enumerate(file_paths):
        eps.append([])
        with open(file_path, 'r') as f:
            for line in f:
                pattern = re.compile(r'eps_(\d+): (\d+\.\d+)')
                match = pattern.search(line)
                if match:
                    eps[i].append(float(match.group(2)))
                else:
                    print('No match')
    data = np.mean(eps, axis=0)

    # 最大值
    maximum = max(data)
    # 最小值
    minimum = min(data)
    # 平均值
    mean = statistics.mean(data)
    # 中位数
    median = statistics.median(data)
    print(f'{maximum:.2f}, {minimum:.2f}, {mean:.2f}, {median:.2f}')

if __name__ == '__main__':
    dir_name = 'data/eps_data/aflnet_static/*'
    gen_result_lab_2(dir_name)
    dir_name = 'data/eps_data/aflnet_user/*'
    gen_result_lab_2(dir_name)
    dir_name = 'data/eps_data/aflnetspy/*'
    gen_result_lab_2(dir_name)