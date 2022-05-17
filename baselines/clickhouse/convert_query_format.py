import re 

datapath = "tpch_queries"
outfile = open("tpch_queries_converted", "w")  # write mode

if __name__ == "__main__": 

    elapsed_to_found_points = {}

    # SELECT * FROM tpch_macro WHERE SHIPDATE BETWEEN 19949069 AND 19950101 AND DISCOUNT BETWEEN 5 AND 7 AND QUANTITY <= 24;, elapsed: 9.582532167434692s, found points: 162727
    regex_template_1 = re.compile(r"WHERE SHIPDATE BETWEEN (?P<ship_date_start>[0-9.]+?) AND (?P<ship_date_end>[0-9.]+?) AND DISCOUNT BETWEEN (?P<discount_start>[0-9.]+?) AND (?P<discount_end>[0-9.]+?) AND QUANTITY <= (?P<quantity_end>[0-9.]+?);")
    regex_template_2 = re.compile(r"WHERE ORDERDATE >= (?P<order_date_start>[0-9.]+?) AND SHIPDATE <= (?P<ship_date_end>[0-9.]+?);")

    with open("{}".format(datapath)) as ifile:

        for line in ifile:
            m = regex_template_1.search(line)
            if m:
                out_line = "0,{},{},2,{},{},4,{},{}".format(-1, m.group("quantity_end"), m.group("discount_start"), m.group("discount_end"), m.group("ship_date_start"), m.group("ship_date_end"))
            else:
                m = regex_template_2.search(line)
                if m:
                    out_line = "4,{},{},8,{},{}".format(-1, m.group("ship_date_end"), m.group("order_date_start"), -1)
            
            outfile.write(out_line + "\n")

    outfile.close()