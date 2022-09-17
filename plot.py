from ctypes import resize
from math import sqrt, dist, pi
import os, threading, subprocess, time
from shutil import ExecError
import sys
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import pandas as pd
import random

def normalize(attributes):
    normalized = []
    for i in range(len(attributes)):
        metrics = []
        for j in range(len(attributes[0])):
            metrics.append(attributes[i][j] / sqrt(sum(attributes[m][j]**2 for m in range(len(attributes)))))
        normalized.append(metrics)
    return normalized

def weight(attributes, metrics_weights):
    weighted = []
    for i in range(len(attributes)):
        metrics = []
        for j in range(len(attributes[0])):
            metrics.append(metrics_weights[j] * attributes[i][j])
        weighted.append(metrics)
    return weighted

def selection(attributes, I):
    best_solution = []
    worst_solution = []
    for j in range(len(attributes[0])):
        best_value = attributes[0][j]
        worst_value = attributes[0][j]
        for i in range(len(attributes)):
            if I[j] == 1:
                best_value = max(best_value, attributes[i][j])
                worst_value = min(worst_value, attributes[i][j])
            else:
                best_value = min(best_value, attributes[i][j])
                worst_value = max(worst_value, attributes[i][j])
        best_solution.append(best_value)
        worst_solution.append(worst_value)
    #print(best_solution, worst_solution)
    distances_from_best = []
    distances_from_worst = []
    for i in range(len(attributes)):
        print(attributes[i])
        distances_from_best.append(dist(attributes[i], best_solution))
        distances_from_worst.append(dist(attributes[i], worst_solution))
    #print(distances_from_best, distances_from_worst)
    coefficients = []
    for i in range(len(attributes)):
        if (distances_from_worst[i] + distances_from_best[i] == 0):
            coefficient = 1
        else:
            coefficient = distances_from_worst[i] / (distances_from_worst[i] + distances_from_best[i])
        coefficients.append(coefficient)

    print("Coefficients ", coefficients)
    return coefficients

number_gateways = [1, 2]
use_case = "building-constraint"
attributes = []
metrics_weights = [0.25, 0.25, 0.25, 0.25]

#results = [ [89.57, 6.38, 0.64, 1750], [100.0, 12.54, 0.23, 1100], [100.0, 15.98, 0.21, 1050]]

#results = [ [100.0, 470.16, 51.73, 74000], [100.0, 471.135, 42.160000000000004, 75000],
#            [95.78, 2327.91, 82.17599999991153, 12000], [99.63, 2327.93, 82.17599999993269, 13000]] 

results = [ #[98.0, 76.85, 0.06999999999999999, 3100], 
            [100.0, 86.07, 0.06999999999999999, 2950],
            [100.0, 86.49, 0.06999999999999999, 3050],
            #  [100.0, 86.67, 0.06999999999999999, 3150],
            [100.0, 351.15, 49.29, 2250], [100.0, 381.408, 49.230000000000004, 3250],
            #[83.5154, 92.2132, 8.17, 3950], 
            [96.9462, 123.989, 6.32, 3400], [98.23, 141.97, 6.32, 3350]]
"""
results = [ [100.0, 470.16, 51.73, 74000], [100.0, 471.13, 42.16, 75000], 
            [95.78, 2327.81, 82.17599999991859, 12000], [99.63, 2327.81, 82.17599999994684, 13000]]
"""

#technologies_configuration = ["WiFi-GW1", "WiFi-GW2", "WiFi-GW3"]
technologies_configuration = ["WiFi-GW2", "WiFi-GW3", "HaLow-GW1", "HaLow-GW2", "6LoWPAN-GW2", "6LoWPAN-GW3"]
#technologies_configuration = ["HaLow-GW1", "HaLow-GW2", "LoRaWAN-GW1", "LoRaWAN-GW2"]

constraint_latency = 100
constraint_lifetime = 90
constraint_delivery = 90

for elem in results:
    if elem[2] < constraint_latency:
        elem[2] = constraint_latency
    if elem[1] < constraint_lifetime:
        elem[1] = 0.1
    if elem[0] < constraint_delivery:
        elem[0] = 0.1

normalized = normalize(results)
weighted = weight(normalized, metrics_weights)

max_delivery = 0
max_lifetime = 0
min_latency = 0
min_cost = 0

I = [1, 1, 0, 0]
result = selection(weighted, I)

for i in range(len(result)):
    print(technologies_configuration[i], results[i], result[i])

for elem in weighted:
    elem[2] = 1 / elem[2]
    elem[3] = 1 / elem[3]

print(weighted)

for i in range(len(weighted)):
    max_delivery = max(max_delivery, weighted[i][0])
    max_lifetime = max(max_lifetime, weighted[i][1])
    min_latency = max(min_latency, weighted[i][2])
    min_cost = max(min_cost, weighted[i][3])

for elem in weighted:
    elem[0] = elem[0] / max_delivery
    elem[1] = elem[1] / max_lifetime
    elem[2] = elem[2] / min_latency
    elem[3] = elem[3] / min_cost

results = list(map(list, zip(*weighted)))

metrics = ['Message Delivery','Battery Lifetime','Latency Efficiency','Cost Efficiency']

df = pd.DataFrame({'group': technologies_configuration})

dictionary = {'group': technologies_configuration}
i = 0
for elem in results:
    dictionary[str(metrics[i])] = elem
    i = i + 1

df = pd.DataFrame(dictionary)

print(df)

deliveries = df['Message Delivery']
max_delivery_norm = max(deliveries)
lifetimes = df['Battery Lifetime']
max_lifetime_norm = max(lifetimes)
latencies = df['Latency Efficiency']
max_latency_norm = max(latencies)
costs = df['Cost Efficiency']
max_cost_norm = max(costs)

max_value = max(max_delivery_norm, max_cost_norm, max_lifetime_norm, max_latency_norm) + 0.02

"""
df = pd.DataFrame({
'group': ['A','B','C','D'],
'var1': [38, 1.5, 30, 4],
'var2': [29, 10, 9, 34],
'var3': [8, 39, 23, 24],
'var4': [7, 31, 33, 14],
'var5': [28, 15, 32, 14]
})
"""
# ------- PART 1: Create background
 
# number of variable
categories=list(df)[1:]
N = len(categories)
 
# What will be the angle of each axis in the plot? (we divide the plot / number of variable)
angles = [n / float(N) * 2 * pi for n in range(N)]
angles += angles[:1]
 
# Initialise the spider plot
ax = plt.subplot(111, polar=True)

# Remove spines
ax.spines["start"].set_color("none")
ax.spines["polar"].set_color("none")

GREY70 = "#b3b3b3"
GREY_LIGHT = "#f2efe8"
GREY50 = "#007A87"
# Angle values going from 0 to 2*pi
HANGLES = np.linspace(0, 2 * np.pi)

# Used for the equivalent of horizontal lines in cartesian coordinates plots 
# The last one is also used to add a fill which acts a background color.
H0 = np.zeros(len(HANGLES))
H1 = np.ones(len(HANGLES)) * max_value
H2 = np.ones(len(HANGLES))

# Add custom lines for radial axis (y) at 0, 0.5 and 1.
#ax.plot(HANGLES, H0, ls=(0, (6, 6)), c=GREY70)
ax.plot(HANGLES, H1, ls=(0, (6, 6)), c=GREY70)
#ax.plot(HANGLES, H2, ls=(0, (6, 6)), c=GREY70)

# Now fill the area of the circle with radius 1.
# This create the effect of gray background.
ax.fill(HANGLES, H1, GREY_LIGHT)

ax.plot([0, 0], [0, max_value], lw=2, c=GREY70)
ax.plot([np.pi, np.pi], [0, max_value], lw=2, c=GREY70)
ax.plot([np.pi / 2, np.pi / 2], [0, max_value], lw=2, c=GREY70)
ax.plot([-np.pi / 2, -np.pi / 2], [0, max_value], lw=2, c=GREY70)
 
# If you want the first axis to be on top:
ax.set_theta_offset(pi / 2)
ax.set_theta_direction(-1)
 
# Draw one axe per variable + add labels
ax.set_xticks(angles[:-1])
ax.set_xticklabels(categories)

# Remove lines for radial axis (y)
ax.set_yticks([])
ax.yaxis.grid(False)
ax.xaxis.grid(False)
 
# Draw ylabels
ax.set_rlabel_position(0)

#colors = ["#"+''.join([random.choice('0123456789ABCDEF') for j in range(6)])
 #            for i in range(len(technologies_configuration)//max(number_gateways))]

colors = ['#449e48', '#92623a', '#ed4d4d', '#26abff']

colorsDict = {
    'WiFi': '#449e48',
    '6LoWPAN': '#92623a',
    'HaLow': '#ed4d4d',
    'LoRaWAN': '#26abff'
}

handles = [
    Line2D(
        [], [], 
        c=color, 
        lw=3, 
        marker="o", 
        markersize=8, 
        label=species
    )
    for species, color in zip(categories, colors)
]
 
legend = ax.legend(
    handles=handles,
    loc=(1, 0),       # bottom-right
    labelspacing=1.5, # add space between labels
    frameon=False     # don't put a frame
)

X_VERTICAL_TICK_PADDING = 2
X_HORIZONTAL_TICK_PADDING = 35

# Adjust tick label positions ------------------------------------
XTICKS = ax.xaxis.get_major_ticks()
for tick in XTICKS[0::2]:
    tick.set_pad(X_VERTICAL_TICK_PADDING)
    
for tick in XTICKS[1::2]:
    tick.set_pad(X_HORIZONTAL_TICK_PADDING)

def cart2pol(x, y):
    rho = np.sqrt(x**2 + y**2)
    phi = np.arctan2(y, x)
    return(rho, phi)

# Add levels -----------------------------------------------------
# These labels indicate the values of the radial axis
#ax.text(max_delivery_norm_polar_x, max_delivery_norm_polar_y, str(max_delivery))
#ax.text(max_latency_norm_polar_x, max_latency_norm_polar_y, str(max_latency))
#ax.text(max_cost_norm_polar_x, max_cost_norm_polar_y, str(max_cost))
#ax.text(max_lifetime_norm_polar_x, max_lifetime_norm_polar_y, str(max_lifetime))
"""
ax.text(np.deg2rad(5), max_delivery_norm, int(max_delivery))
ax.text(np.deg2rad(175), max_latency_norm, int(max_latency))
ax.text(np.deg2rad(85), max_lifetime_norm, int(max_lifetime))
ax.text(np.deg2rad(275), max_cost_norm + 0.05, int(max_cost))
"""
#ax.text(-0.4, 0.5 + PAD, "")
#ax.text(-0.4, 1 + PAD, "")

# ------- PART 2: Add plots

linestyles = [ ":", "--", "-", "dotted", "solid", "dashed", "dashdot", "-."] 
 
# Plot each individual = each line of the data
# I don't make a loop, because plotting more than 3 groups makes the chart unreadableins
 
for i in range(len(technologies_configuration)):
    values=df.loc[i].drop('group').values.flatten().tolist()
    values += values[:1]
    color = colorsDict[technologies_configuration[i].split('-')[0]]
    ax.plot(angles, values, linewidth=1, color=color, linestyle=linestyles[i%max(number_gateways)], label=technologies_configuration[i])
    ax.fill(angles, values, alpha=0.1)

# Add legend
plt.legend(loc=(0.98,0.62), prop={'size': 8.5})

# Show the graph

plt.savefig(str(use_case)+'.png')