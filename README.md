

## Setup

- Create `/proj` and set permission:
```bash
sudo mkdir /proj
sudo chown -R <your_user_name>:<your_user_name> /proj
sudo chmod -R 755 /proj
```

- Put **this** repo in `/proj/Trinity`
- Run
  ```bash
  cd /proj/Trinity
  bash scripts/setup_one_node.sh
  ```


## Run Benchmark

- Put [`trinitoxide`](https://github.com/tsunrise/trinitoxide) repo in `/proj/trinitoxide`
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
  cd /proj/trinitoxide
  python3 scripts/download_github_dataset.py -n <number_of_points> -o data/github.csv
  ```

  Set `number_of_points` to be the maximum number of points you want to use in the benchmark. For example, if in benchmark you want to test `10000`, `100000`, `1000000` points, you should set `number_of_points` to be `1000000`.

- Link dataset to this repo:
  ```bash
  cd /proj/Trinity
  mkdir -p datasets
  cd datasets
  ln -s /proj/trinitoxide/data/github.csv github.csv
  ```

- Run benchmark for CPP:
  ```bash
  cd /proj/Trinity
  cd build
  make
  ./libmdtrie/microbench -m <number_of_points> -n <number_of_lookup_queries> -r <number_of_range_queries>
  ```
