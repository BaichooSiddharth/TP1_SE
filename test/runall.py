import contextlib
import os
import re
import subprocess
import yaml

with open("in_out.yaml") as f:
    tests_yaml = yaml.safe_load(f)
dico = {}

def clean_strings(list):
    return [bytes(str, "utf-8").decode("unicode_escape") for str in list]

weight_sum = 0
for testcase, diccc in tests_yaml.items():
    dico[testcase] = diccc["weight"]
    weight_sum += diccc["weight"]
    #dico["in"] = clean_strings(dico["in"])
    #dico["out"] = clean_strings(dico["out"])

for key, weight in dico.items():
    dico[key] = weight / weight_sum

@contextlib.contextmanager
def directory(name):
    ret = os.getcwd()
    os.chdir(name)
    yield None
    os.chdir(ret)

def run(input):
    input = input+"exit\n"
    try:
        program_output = subprocess.check_output(f"echo \"{input}\" - | valgrind --leak-check=full --leak-resolution=med --trace-children=no --track-origins=yes --vgdb=no --log-file=\"valgrind.log\" ./check_tests", shell=True, universal_newlines=True, stderr=subprocess.STDOUT)
    except Exception as e:
        program_output = e.output

    with open("./valgrind.log", "r") as f:
        valgrind_output = f.read()

    return program_output, valgrind_output

# tests
perfect_score = {key:0 for (key, _) in dico.items()}
points = perfect_score.copy()

failed = 0
valgrind_output = ""
for test_case, inout in tests_yaml.items():
    for input, output in zip(inout["in"], inout["out"]):
        with directory("tests_build"):
            program_output, valgrind = run(input)
        valgrind_output += valgrind + "\n\n\n"

        perfect_score[test_case] += 1

        a = len(program_output)
        b = len(output)

        if program_output.strip() == output.strip():
            points[test_case] += 1

# valgrind
def pts_lost_for_mem_leaks(val):
    if len(re.findall(r"(definitely lost|indirectly lost): [1-9]", val)) == 0:
        return 0
    return 15

def pts_lost_for_invalids(val):
    return min(val.count("invalid read of size") + val.count("invalid write of size"), 5)

try:
    pts_lost = -(pts_lost_for_mem_leaks(valgrind_output) + pts_lost_for_invalids(valgrind_output)) / 100
except Exception:
    pts_lost = -0.2
print(f"Points lost with Valgrind: {pts_lost}")
total = pts_lost

for key, val in dico.items():
    print(f"{key}: {(points[key] / perfect_score[key])}")
    total += (points[key] / perfect_score[key])*val
print(f"GRADE:{{{total}}}")