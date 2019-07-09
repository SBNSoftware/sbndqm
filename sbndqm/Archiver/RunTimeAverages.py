import csv

def Mean(l):
    return sum(l)/len(l)

with open('logs/times.log', 'r') as Input:
    Reader = csv.reader(Input)
    Times = [ float(x[0]) for x in list(Reader) ]

print("Average time: " + str(Mean(Times)))
