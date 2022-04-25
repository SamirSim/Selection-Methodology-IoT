import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

sns.set_style("whitegrid")

data = pd.read_csv("surveillance/success-rate/Success-Rate-point-copie.csv", delimiter=";")

#print(data)

data = data.melt('Nsta', var_name='Configurations',  value_name='Success Rate (%)')
g = sns.catplot(x="Nsta", y="Success Rate (%)", hue='Configurations', data=data, kind='point')

g.set(xlabel='Number of cameras', ylabel='Success Rate (%)')

plt.show()

data = pd.read_csv("surveillance/energy/Ratio-Global-point-tronque-inverse.csv", delimiter=";")

#print(data)

data = data.melt('Nsta', var_name='Configurations',  value_name='Energy Efficiency Ratio (MegaBytes/J)')
g = sns.catplot(x="Nsta", y="Energy Efficiency Ratio (MegaBytes/J)", hue='Configurations', data=data, kind='point')

g.set(xlabel='Number of cameras', ylabel='Energy Efficiency Ratio (MegaBytes/J)')

plt.show()

data = pd.read_csv("surveillance/energy/Lifetime-Global-point-tronque.csv", delimiter=";")

data = data.melt('Nsta', var_name='Configurations',  value_name='Battery Lifetime (Days)')
g = sns.catplot(x="Nsta", y="Battery Lifetime (Days)", hue='Configurations', data=data, kind='point')

g.set(xlabel='Number of cameras', ylabel='Battery Lifetime (Days)')

plt.show()

data = pd.read_csv("surveillance-latency/latency-copie/Latency-Global-points-9-all-points-copie-step3.csv", delimiter=";")

data = data.melt('Nsta', var_name='Configurations',  value_name='Packet Latency (µS)')
g = sns.catplot(x="Nsta", y="Packet Latency (µS)", hue='Configurations', data=data, kind='point')

g.set(xlabel='Number of cameras', ylabel='Packet Latency (µS)')
g.set(ylim=(0, 6))

#plt.xticks(rotation="45")
plt.show()
