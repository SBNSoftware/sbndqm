import csv

def Mean(l):
    return sum(l)/len(l)

with open('logs/times10.log', 'r') as Input:
    Reader = csv.reader(Input)
    Times10 = [ float(x[0]) for x in list(Reader) ]
with open('logs/times100.log', 'r') as Input:
    Reader = csv.reader(Input)
    Times100 = [ float(x[0]) for x in list(Reader) ]
with open('logs/times1000.log', 'r') as Input:
    Reader = csv.reader(Input)
    Times1000 = [ float(x[0]) for x in list(Reader) ]
with open('logs/times10000.log', 'r') as Input:
    Reader = csv.reader(Input)
    Times10000 = [ float(x[0]) for x in list(Reader) ]

Times = [Mean(Times10), Mean(Times100), Mean(Times1000), Mean(Times10000)]

print(Times)
