

## Setup

```bash
bash scripts/setup_one_node.sh
```


## Run Benchmark
- Put **this** repo in `/proj/Trinity`
- Put `trinioxide` repo in `/proj/trinioxide`
- In this repo, setup:
  ```bash
  cd /proj/Trinity
  ./scripts/setup_one_node.sh
  ```
- In this repo, build:

  ```bash
  cd /proj/Trinity
  mkdir -p build
  cd build
  cmake ..
  make
  ```

- In trinioxide repo, download dataset:
  ```bash
  cd /proj/trinioxide
  python3 scripts/download_github_dataset.py -n 1000000 -o data/github.csv
  ```

- Link dataset to this repo:
  ```bash
  cd /proj/Trinity
  mkdir -p datasets
  cd datasets
  ln -s /proj/trinioxide/data/github.csv github.csv
  ```

- Run benchmark:
  ```bash
  cd /proj/Trinity
  cd build
  make
  ./libmdtrie/microbench -b github
  ```
