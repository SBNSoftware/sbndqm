import argparse
import redis

def main(args):
    r = redis.Redis(host=args.hostname, port=args.port, password=args.password)
    for stream in args.stream:
        sum = 0
        val = r.xrange(stream)
        for item in val:
            sum += float(item[1]["dat"])
        print("Average: " + str(sum / len(val))) 

def comma_list(inp):
    return [x for x in inp.split(",") if len(x) > 0]

if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args.add_argument("-s", "--stream", required=True, type=comma_list)
    args.add_argument("-t", "--timeout", default=1, type=float)
    args.add_argument("-hn", "--hostname", default="localhost")
    args.add_argument("-p", "--port", default=6379, type=int)
    args.add_argument("-pw", "--password", default=None)

    main(args.parse_args())
