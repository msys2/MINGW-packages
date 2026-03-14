#!/usr/bin/env python3
#
# Compute a list of modified packages and check each package by reading PKGBUILD, what msystem architectures are
# required for building packages.
#
# Outputs:
#  jobmatrix        - A JSON encoded array of dictionaries containing: 'icon', 'msystem' and 'runner' items.
#  modifiedPackages - A space separated list of modified package names.
#
from json       import dumps
from os         import getenv
from pathlib    import Path
from re         import compile, MULTILINE
from subprocess import check_output
from textwrap   import dedent

availableJobs = [
  {"icon": "🟨", "msystem": "UCRT64",     "runner": "windows-2022"},
  {"icon": "🟧", "msystem": "CLANG64",    "runner": "windows-2022"},
  {"icon": "🟦", "msystem": "MINGW64",    "runner": "windows-2022"},
  {"icon": "⬛", "msystem": "MINGW32",    "runner": "windows-2022"},
  {"icon": "🟪", "msystem": "CLANGARM64", "runner": "windows-11-arm"}
]
defaultMinGWArch = {"mingw32", "mingw64", "ucrt64", "clang64"}

def git_log(diff) -> list:
  output = check_output(["git", "log", "--pretty=format:", "--name-only", diff]).decode("utf-8").strip()
  return output.splitlines()

changes =  set(Path(file) for file in git_log("upstream/master.."))
# changes |= set(Path(file) for file in git_log("HEAD^.."))                  # Delivers same result as previous command. What's the use case?
# changes = list(dict.fromkeys(x.split("::")[-1] for x in sorted(changes)))  # What's the use case?

print(f"Modified files:")
for change in changes:
  print(f"  {change}")

modifiedPackages = [path.parent for path in changes if path.exists() and path.name == "PKGBUILD"]

print("Modified packages:")
for packagePath in modifiedPackages:
  print(f"  {packagePath.name}")

architectures = set()
pattern = compile(r"""^mingw_arch=\(('|")(.*?)('|")\)""", flags=MULTILINE)
print("Inspecting PKGBUILD files for 'mingw_arch' settings ...")
for packagePath in modifiedPackages:
  pkgBuildPath = packagePath / "PKGBUILD"
  print(f"  {pkgBuildPath}")
  with pkgBuildPath.open("r", encoding="utf-8") as f:
    content = f.read()

  if (match := pattern.search(content)) is not None:
    print(f"    {match[0]}")
    architectures |= set(arch.replace("'", "").lower() for arch in match[2].split(" ") if arch != "")
  else:
    print("    'mingw_arch' not configured => using defaults")
    architectures |= defaultMinGWArch
  
  if len(architectures) == len(availableJobs):
    if architectures == {job["msystem"].lower() for job in availableJobs}:
      print("All build architectures are needed.")
      break
    else:
      print("Collected an unknown architecture!")
      print(f"::error title=Unknown Architecture::While collecting necessary architectures, an unknown architecture was discovered. {architectures}")
      exit(1)

print("Necessary build architectures:")
for architecture in architectures:
  print(f"  {architecture}")

jobs = [job for job in availableJobs if job["msystem"].lower() in architectures]
print("Jobs to start:")
for job in jobs:
  print(f"  {job["icon"]} {job["msystem"]} on {job["runner"]}")

githubOutput = Path(getenv("GITHUB_OUTPUT"))
print(f"GITHUB_OUTPUT: {githubOutput}")
with githubOutput.open("a+", encoding="utf-8") as f:
  f.write(dedent(f"""\
    jobmatrix={dumps(jobs)}
    modifiedPackages={' '.join(str(package) for package in modifiedPackages)}
  """))

if len(modifiedPackages) == 0 or len(jobs) == 0:
  print(f"::error title=No Action Required::There have been no modifications to packages.")
  exit(1)
