import aerospike
import sys
import csv
import re
from aerospike import predicates
from aerospike import exception as ex
from aerospike_helpers import expressions as exp
import time
import multiprocessing
from multiprocessing import Process
from concurrent.futures import ProcessPoolExecutor
from threading import Thread, Lock
import fcntl

config = {
  'hosts': [ ('10.10.1.12', 3000),('10.10.1.13', 3000),('10.10.1.14', 3000),('10.10.1.15', 3000),('10.10.1.16', 3000)]
}


# filename = "/proj/trinity-PG0/Trinity/results/github_clickhouse"
filename = "/proj/trinity-PG0/Trinity/queries/github/github_query_final"
total_points = int(50000000) # 50M
stars_range = [0, 354850]  # min, max
forks_range = [0, 262926]  # min, max
issues_range = [0, 379379]  # min, max
events_count_range = [1, 7451541]  # min, max
start_date_range = [20110211, 20201206]  # min, max
end_date_range = [20110211, 20201206]  # min, max

regex_template_1 = re.compile(r"where start_date <= (?P<start_date_end>[0-9.]+?) AND end_date >= (?P<end_date_start>[0-9.]+?) AND stars >= (?P<stars_start>[0-9.]+?),")
regex_template_2 = re.compile(r"where stars >= (?P<stars_start>[0-9.]+?) and forks >= (?P<forks_start>[0-9.]+?),")
regex_template_3 = re.compile(r"where events_count <= (?P<events_count_end>[0-9.]+?) and issues >= (?P<issues_start>[0-9.]+?) AND stars >= (?P<stars_start>[0-9.]+?),")
regex_template_4 = re.compile(r"where start_date >= (?P<start_date_start>[0-9.]+?) AND start_date <= (?P<start_date_end>[0-9.]+?) AND end_date >= (?P<end_date_start>[0-9.]+?) AND end_date <= (?P<end_date_end>[0-9.]+?),")

with open(filename) as file:
    lines = file.readlines()
    lines = [line.rstrip() for line in lines]

processes = []
total_queries = 100 # should be 500

def query_each_worker(worker_idx, total_workers):

    # Create a client and connect it to the cluster
    try:
        client = aerospike.client(config).connect()
        print("client connected")
    except:
        import sys
        print("failed to connect to the cluster with", config['hosts'])
        sys.exit(1)

    start_time = time.time()
    effective_pts_count = 0
    for i in range(worker_idx, total_queries, total_workers):
        
        line = lines[i]
        line = line.split(";,")[0] + ";"
        line = line.replace("COUNT(*)", "*")

        query = client.query('macro_bench', 'github_macro')
        query.select("events_count", "authors_count", "forks", "stars", "issues", "pushes", "pulls", "downloads", "start_date", "end_date")
        picked_template = 1
        for reg in [regex_template_1, regex_template_2, regex_template_3, regex_template_4]:
            m = reg.search(line)
            if m:
                break
            picked_template += 1
        if not m:
            print("error!", line)
            exit(0)

        attribute_to_selectivity = {}

        # Picking other fields are always the best
        '''
        if (picked_template == 1) and int(m.group("start_date_end")) != start_date_range[1]:
            attribute_to_selectivity["start_date"] = (int(m.group("start_date_end")) - start_date_range[0]) / (start_date_range[1] - start_date_range[0])

        if (picked_template == 1) and int(m.group("end_date_start")) != end_date_range[0]:
            attribute_to_selectivity["end_date"] = (end_date_range[1] - int(m.group("end_date_start"))) / (end_date_range[1] - end_date_range[0])
        '''

        if (picked_template == 1 or picked_template == 2 or picked_template == 3) and int(m.group("stars_start")) != stars_range[0]:
            attribute_to_selectivity["stars"] = (stars_range[1] - int(m.group("stars_start"))) / (stars_range[1] - stars_range[0])

        if (picked_template == 2) and int(m.group("forks_start")) != forks_range[0]:
            attribute_to_selectivity["forks"] = (forks_range[1] - int(m.group("forks_start"))) / (forks_range[1] - forks_range[0])

        '''
        if (picked_template == 3) and int(m.group("events_count_end")) != events_count_range[1]:
            attribute_to_selectivity["events_count"] = (int(m.group("events_count_end")) - events_count_range[0]) / (events_count_range[1] - events_count_range[0])
        '''
        
        if (picked_template == 3) and int(m.group("issues_start")) != issues_range[0]:
            attribute_to_selectivity["issues"] = (issues_range[1] - int(m.group("issues_start"))) / (issues_range[1] - issues_range[0])

        if (picked_template == 4):
            attribute_to_selectivity["start_date"] = (int(m.group("start_date_end")) - int(m.group("start_date_start"))) / (start_date_range[1] - start_date_range[0])
            attribute_to_selectivity["end_date"] = (int(m.group("end_date_end")) - int(m.group("end_date_start"))) / (end_date_range[1] - end_date_range[0])     

        picked_query = min(attribute_to_selectivity, key=attribute_to_selectivity.get)
        print(line, picked_query, attribute_to_selectivity[picked_query])

        if picked_query == "start_date":
            # query.where(predicates.between('start_date', start_date_range[0], int(m.group("start_date_end"))))
            # print('query:', 'start_date', start_date_range[0], int(m.group("start_date_end")))
            query.where(predicates.between('start_date', int(m.group("start_date_start")), int(m.group("start_date_end"))))
            # print('query:', 'start_date', int(m.group("start_date_start")), int(m.group("start_date_end")))

        elif picked_query == "end_date":
            # query.where(predicates.between('end_date', int(m.group("end_date_start")), end_date_range[1]))
            # print('query:', 'end_date', int(m.group("end_date_start")), end_date_range[1])
            query.where(predicates.between('end_date', int(m.group("end_date_start")), int(m.group("end_date_end"))))
            # print('query:', 'end_date', int(m.group("end_date_start")), int(m.group("end_date_end")))

        elif picked_query == "stars":
            query.where(predicates.between('stars', int(m.group("stars_start")), stars_range[1]))
            # print('query:', 'stars', int(m.group("stars_start")), stars_range[1])

        elif picked_query == "forks":
            query.where(predicates.between('forks', int(m.group("forks_start")), forks_range[1]))
            # print('query:', 'forks', int(m.group("forks_start")), forks_range[1])

        elif picked_query == "events_count":
            query.where(predicates.between('events_count', events_count_range[0], int(m.group("events_count_end"))))
            # print('query:', 'events_count', events_count_range[0], int(m.group("events_count_end")))

        elif picked_query == "issues":
            query.where(predicates.between('issues', int(m.group("issues_start")), issues_range[1]))
            # print('query:', 'issues', int(m.group("issues_start")), issues_range[1])

        else:
            print("wrong!")
            exit(0)

        if picked_template == 1:

            '''
            regex_template_1 = re.compile(r"where start_date <= (?P<start_date_end>[0-9.]+?) AND end_date >= (?P<end_date_start>[0-9.]+?) AND stars >= (?P<stars_start>[0-9.]+?),")
            '''

            # print(picked_template, int(m.group("end_date_start")), int(m.group("start_date_end")), int(m.group("stars_start")))
            if picked_query == "start_date":

                expr = exp.And(
                    exp.GE(exp.IntBin("end_date"), int(m.group("end_date_start"))),
                    exp.GE(exp.IntBin("stars"), int(m.group("stars_start"))),
                ).compile()

            if picked_query == "end_date":

                expr = exp.And(
                    exp.LE(exp.IntBin("start_date"), int(m.group("start_date_end"))),
                    exp.GE(exp.IntBin("stars"), int(m.group("stars_start"))),
                ).compile()

            if picked_query == "stars":

                expr = exp.And(
                    exp.LE(exp.IntBin("start_date"), int(m.group("start_date_end"))),
                    exp.GE(exp.IntBin("end_date"), int(m.group("end_date_start"))),
                ).compile()

        if picked_template == 2:

            '''
            regex_template_2 = re.compile(r"where stars >= (?P<stars_start>[0-9.]+?) and forks >= (?P<forks_start>[0-9.]+?),")
            '''

            # print(picked_template, int(m.group("forks_start")), int(m.group("stars_start")))

            if picked_query == "stars":

                expr = exp.GE(exp.IntBin("forks"), int(m.group("forks_start"))).compile()

            if picked_query == "forks":

                expr = exp.GE(exp.IntBin("stars"), int(m.group("stars_start"))).compile()

        if picked_template == 3:

            '''
            regex_template_3 = re.compile(r"where events_count <= (?P<events_count_end>[0-9.]+?) and issues >= (?P<issues_start>[0-9.]+?) AND stars >= (?P<stars_start>[0-9.]+?),")
            '''

            # print(picked_template, int(m.group("issues_start")), int(m.group("stars_start")), int(m.group("events_count_end")))

            if picked_query == "events_count":
                expr = exp.And(
                    exp.GE(exp.IntBin("issues"), int(m.group("issues_start"))),
                    exp.GE(exp.IntBin("stars"), int(m.group("stars_start"))),
                ).compile()

            if picked_query == "issues":
                expr = exp.And(
                    exp.LE(exp.IntBin("events_count"), int(m.group("events_count_end"))),
                    exp.GE(exp.IntBin("stars"), int(m.group("stars_start"))),
                ).compile()

            if picked_query == "stars":
                expr = exp.And(
                    exp.LE(exp.IntBin("events_count"), int(m.group("events_count_end"))),
                    exp.GE(exp.IntBin("issues"), int(m.group("issues_start"))),
                ).compile()


        if picked_template == 4:

            '''
            regex_template_4 = re.compile(r"where start_date >= (?P<start_date_start>[0-9.]+?) AND start_date <= (?P<start_date_end>[0-9.]+?) AND end_date >= (?P<end_date_start>[0-9.]+?) AND end_date <= (?P<end_date_end>[0-9.]+?);")
            '''

            # print(picked_template, int(m.group("start_date_start")), int(m.group("start_date_end")), int(m.group("end_date_start")), int(m.group("end_date_end")))

            if picked_query == "start_date":
                expr = exp.And(
                    exp.GE(exp.IntBin("end_date"), int(m.group("end_date_start"))),
                    exp.LE(exp.IntBin("end_date"), int(m.group("end_date_end"))),
                ).compile()

            if picked_query == "end_date":
                expr = exp.And(
                    exp.GE(exp.IntBin("start_date"), int(m.group("start_date_start"))),
                    exp.LE(exp.IntBin("start_date"), int(m.group("start_date_end"))),
                ).compile()


        policy = {
            'expressions': expr,
            'total_timeout':200000,
            'socket_timeout': 200000
        }

        records_query = query.results(policy)
        effective_pts_count += len(records_query)

        print("i: ", i, "len(results)", len(records_query))
        del records_query
    end_time = time.time()
    return effective_pts_count / (end_time - start_time)

def query_worker(worker, total_workers, return_dict):

    throughput = query_each_worker(worker, total_workers)
    return_dict[worker] = {"throughput": throughput}

manager = multiprocessing.Manager()
return_dict = manager.dict()
threads_per_node = 20

if int(sys.argv[1]) == 0:
    with open('/proj/trinity-PG0/Trinity/baselines/aerospike/python/github_search_throughput.txt', 'a') as f:
        fcntl.flock(f, fcntl.LOCK_EX)
        print("---- {}K ----".format(int(total_points / 1000)), file=f)
        fcntl.flock(f, fcntl.LOCK_UN)

for worker in range(threads_per_node):
    p = Process(target=query_worker, args=(worker, threads_per_node, return_dict, ))
    p.start()
    processes.append(p)

for p in processes:
    p.join()

print("total throughput", sum([return_dict[worker]["throughput"] for worker in return_dict]))

with open('/proj/trinity-PG0/Trinity/baselines/aerospike/python/github_search_throughput.txt', 'a') as f:
    fcntl.flock(f, fcntl.LOCK_EX)
    print(sum([return_dict[worker]["throughput"] for worker in return_dict]), file=f)
    fcntl.flock(f, fcntl.LOCK_UN)
