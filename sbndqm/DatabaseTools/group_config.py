import argparse
import util
import json


def main(args):
    redis = util.connect_to_redis_args(args)
    try:
        redis.ping()
    except:
        print "Error: trouble connecting to redis database on server (%s) port (%i)" % (args.server, args.port)
        return
    try:
        config = json.loads(redis.get("GROUP_CONFIG:%s" % args.group))
    except:
        print "Error: configuration for group %s does not exist (key GROUP_CONFIG:%s either does not exist or is mis-formed)" % (args.group, args.group)
        return 
    metrics = config["metric_config"].keys()
    streams = config["streams"]
    print "Group %s is configured for:" % args.group
    print "Metrics:" + ",".join([" %s" % m for m in metrics])
    print "Streams:" + ",".join([" %s" % s for s in streams])

    try:
       members = redis.lrange("GROUP_MEMBERS:%s" % args.group, 0, -1)
    except:
        print "Error: members for group %s do not exist (key GROUP_MEMBERS:%s either does not exist or is mis-formed)" % (args.group, args.group)
    print "Members:" + ",".join([" %s" % m for m in members])

if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args = util.add_connection_args(args)
    args.add_argument("-g", "--group", required=True)
    main(args.parse_args())
