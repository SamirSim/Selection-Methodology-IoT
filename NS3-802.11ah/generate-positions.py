from sklearn.cluster import KMeans
import pandas as pd
import matplotlib.pyplot as plt
import math
import random
import sys

rho = int(sys.argv[1])
nSta = int(sys.argv[2])
nGw = int(sys.argv[3])
filename = str(sys.argv[4])

x = rho
y = rho
cpt = 0

f = open(filename, "w")
f.write("x,y,z\n")
f.close()

f = open(filename, "a")
while cpt < nSta:
    while math.sqrt(x*x + y*y) > rho:
        
        x = random.uniform(-rho, rho)
        y = random.uniform(-rho, rho)
    string = str(x)+','+str(y)+',0\n'
    f.write(string)
    x = rho
    y = rho
    cpt = cpt + 1
f.close()

data = pd.read_csv(filename, delimiter=",")

print(data)

kmeans = KMeans(n_clusters=nGw).fit(data)

pred = kmeans.predict(data)
print(kmeans.labels_)

var = kmeans.cluster_centers_
f = open(filename, "a")
for elem in var:   
    string = ','.join(str(e) for e in elem)
    f.write(str(string)+'\n')
f.close()

f = open(filename, "a")
for elem in pred:   
    f.write(str(elem)+'\n')
f.close()

plt.scatter(data["x"], data["y"], c=pred)
plt.scatter(var[:, 0], var[:, 1], c="red")
plt.show()
