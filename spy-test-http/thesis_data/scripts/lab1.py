import glob
import re
import statistics
import numpy as np

def extreact_trace_bits_from_dir(dir_name):
    file_paths = glob.glob(dir_name)
    file_paths = sorted(file_paths, key=lambda x: int(x.split("/")[-1].split(".")[0].split("_")[-1]))
    res = {}
    for i, file_path in enumerate(file_paths):
        with open(file_path, "r") as file:
            res[i+1] = {}
            for line in file:
                if "count" in line:
                    continue
                if "trace_bits" in line:
                    pattern = r"trace_bits\[(\d+)\]: (\d+)"
                    matches = re.findall(pattern, line)
                    if matches:
                        pos = int(matches[0][0])
                        count = int(matches[0][1])
                        res[i+1][pos] = count
    return res

def compare_trace_bits(dict1, dict2):
    keys1 = set(dict1.keys())
    keys2 = set(dict2.keys())
    keys = keys1.union(keys2)
    count = 0
    for key in keys:
        if key in dict1 and key in dict2 and dict1[key] == dict2[key]:
            count += 1
    
    return count / len(keys)

def get_result(data):
    # 最大值
    maximum = max(data)
    # 最小值
    minimum = min(data)
    # 平均值
    mean = statistics.mean(data)
    # 中位数
    median = statistics.median(data)
    # 众数
    mode = statistics.mode(data)
    return maximum, minimum, mean, median, mode

def gen_result_lab_1(dir_names, thresholds, log_file):
    # 初始化
    results = {}
    for threshold in thresholds:
        results[threshold] = []

    print("=====================================", file=log_file)
    print("最大值\t最小值\t平均值\t中位数\t众数", file=log_file)
    for dir_name in dir_names:
        res = extreact_trace_bits_from_dir(dir_name)
        print(f"{dir_name}:", file=log_file)

        for threshold in thresholds:
            family_count = {}
            for i in range(1, len(res)+1):
                family_count[i] = 0
                for j in range(1, len(res)+1):
                    if compare_trace_bits(res[i], res[j]) > threshold:
                        family_count[i] += 1
            maximum, minimum, mean, median,mode_value = get_result(list(family_count.values()))
            results[threshold].append((maximum, minimum, mean, median, mode_value))
            print(f"threshold: {threshold}\t{maximum:.2f}\t{minimum:.2f}\t{mean:.2f}\t{median:.2f}\t{mode_value:.2f}", file=log_file)
        del res

    print("最终结果: ", file=log_file)
    for threshold in thresholds:
        # 计算平均值
        maximum, minimum, mean, median,mode_value = np.mean(results[threshold],axis=0)
        print(f"threshold: {threshold}\t{maximum:.2f}\t{minimum:.2f}\t{mean:.2f}\t{median:.2f}\t{mode_value:.2f}", file=log_file)

if __name__ == '__main__':
    thresholds = [0.5, 0.6, 0.7, 0.8, 0.9, 0.95, 0.99, 0.999, 0.9999]

    # dir_names_user = ["data/trace_bits_data/aflnet_user/afl-spy-logs-1/*",
    #              "data/trace_bits_data/aflnet_user/afl-spy-logs-2/*",
    #              "data/trace_bits_data/aflnet_user/afl-spy-logs-3/*"]
    
    # log_file = open("log-user.txt", "w")
    # gen_result_lab_1(dir_names_user, thresholds, log_file)
    # log_file.close()

    dir_names_system = ["data/trace_bits_data/aflnetspy/afl-spy-logs-1/*",
                        "data/trace_bits_data/aflnetspy/afl-spy-logs-2/*",
                        "data/trace_bits_data/aflnetspy/afl-spy-logs-3/*"]
    
    log_file = open("log-system.txt", "w")
    gen_result_lab_1(dir_names_system, thresholds, log_file)
    log_file.close()

    # dir_names_static = ["data/trace_bits_data/aflnet_static/afl-spy-logs-1/*",
    #                     "data/trace_bits_data/aflnet_static/afl-spy-logs-2/*",
    #                     "data/trace_bits_data/aflnet_static/afl-spy-logs-3/*"]
    
    # log_file = open("log-static.txt", "w")
    # gen_result_lab_1(dir_names_static, thresholds, log_file)
    # log_file.close()
