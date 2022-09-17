import pandas as pd
import sys

nSta = sys.argv[1]
dataRate = sys.argv[2]
MCS = sys.argv[3]
payloadSize = sys.argv[4]

dfs = []

for filenum in range(1,int(nSta)+1):
	try:
		dfs.append( pd.read_csv(dataRate+"-"+MCS+"-"+payloadSize+"-"+'{}.csv'.format(filenum)) )
	except:
		pass

print(pd.concat(dfs,axis=1).to_csv(index=False))