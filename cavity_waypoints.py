# -*- coding: utf-8 -*-

# for trace analysis
import scipy.ndimage as ndi
import numpy as np
import matplotlib.pyplot as plt
from collections import deque
import copy
import time

def v2i(v):
    return int((v+200)/400*999999)

def i2v(i):
    return i/2500.0-200.0

def lockpoints_function(voltage, transm, errors, sign_flank, rel_threshold, offset_shift):
    # calculate derivative/gradient
    grad = errors[1:]-errors[:-1]
    grad = ndi.gaussian_filter1d(grad, 25)
    
    # get steepest gradient
    strongest = np.max(np.abs(grad))
    threshold = rel_threshold * strongest
    
    # Identify gradients that are relatively steep, from there move left and right until an extremum is reached.
    # This marks the capture range of the lock.
    flanks = []
    grad_copy = np.abs(grad)
    while(np.max(grad_copy)>=threshold):
        # find biggest flank remaining
        max_pos = np.argmax(grad_copy)
        slope = np.sign(grad[max_pos])
        # find left edge of flank
        i = max_pos
        while i>0 and slope*grad[i]>0:
            i -= 1
        ll = i + 1
        # find right edge of flank
        i = max_pos
        while i<len(grad_copy) and slope*grad[i]>0:
            i += 1
        ul = i - 1
        # abort when intervals become zero
        if ul==ll:
            break
        # save flank
        flanks.append([ll,ul, max_pos, slope])
        # remove flank and some extra space from data
        span = ul-ll
        grad_copy[np.clip(ll-2*span,0,len(grad)-1):np.clip(ul+2*span, 0, len(grad)-1)] = 0
        
    # Find triplets of lines that are equally spaced
    # helper function to allow for certain tolerance in distance measurement:
    def within_tolerance(a, b, t=0.05):
        if a/(1-t) > b > a*(1-t):
            return True
        else:
            return False
    # walk through all combinations of three lines
    n = len(flanks)
    used = [False]*n
    triplets = []
    for i in np.arange(0,n-2):
        for j in np.arange(i+1,n-1):
            for k in np.arange(j+1, n):
                if used[i] or used[j] or used[k]:
                    continue
                positions = np.sort([voltage[flanks[i][2]],voltage[flanks[j][2]],voltage[flanks[k][2]]])
                dist1 = positions[1]-positions[0]
                dist2 = positions[2]-positions[1]
                if within_tolerance(dist1,dist2) and within_tolerance(dist1,0.4,0.5):
                    used[i] = True
                    used[j] = True
                    used[k] = True
                    triplets.append([i,j,k,positions[0],positions[1],positions[2]])
        
    # Now get the lockpoints for each triplet.
    lockpoints = []
    for triplet in triplets:
        span = triplet[5]-triplet[3]
        fig, ax = plt.subplots(2,1,figsize=(12,5))
        ax[0].plot(voltage, transm)
        ax[1].plot(voltage, errors)
        limits = [triplet[3]-0.3*span,triplet[5]+0.3*span]
        ax[0].set_xlim(limits)
        ax[1].set_xlim(limits)
        
        for i in triplet[:3]:
            if flanks[i][3] == sign_flank:
                offset = (errors[flanks[i][0]]+errors[flanks[i][1]])/2+offset_shift
                center = flanks[i][0]+np.argmin(np.abs(errors[flanks[i][0]:flanks[i][1]]-offset))
                lockpoints.append([center, offset, sign_flank])
                ax[1].scatter(center, offset, c='k')
        
        plt.show()
    
    return lockpoints, flanks, triplets
    



def get_all_transmission_lines_larger(voltage, transm, index, direction, t=0.1):
    # define transmission offset
    baseline = np.mean(transm)
    # transmission level
    level = transm[index]
    # threshold transmission level
    threshold = level - t * (level-baseline)
    # define array boundaries
    lower = 0 if direction>0 else index
    upper = (index+1) if direction>0 else len(transm)-1
    # make a copy of transmission, so that we can modify it
    transm_copy = copy.copy(transm)
    
    # identify all relevant maxima
    maxima = []
    while np.max(transm_copy[lower:upper]) > threshold:
        next_highest = np.argmax(transm_copy[lower:upper])+lower
        # find eges of transmission peak
        ll = next_highest
        while transm_copy[ll]-baseline > 0.05*(transm_copy[next_highest]-baseline):
            ll -= 1
        ul = next_highest
        while transm_copy[ul]-baseline > 0.05*(transm_copy[next_highest]-baseline):
            ul += 1
        # add to list
        maxima.append(next_highest)
        # remove from transmission copy
        transm_copy[ll:ul] = baseline
        '''
        # adjust threshold in case there is a transmission lower than the level but higher then the threshold
        if transm[next_highest]<level:
            print("adjusted level")
            new_level = transm[next_highest]
            threshold = new_level - t * (new_level-baseline)
        '''

    return sorted(maxima, reverse=(True if direction<0 else False))

CARRIER = 0
LEFT_SIDEBAND = -1
RIGHT_SIDEBAND = +1
GO_TO = 2
GO_UP = 3
GO_DOWN = 4
ERRORS_SMALLER = -1
ERRORS_LARGER = 1
TRANSM_LARGER = 5
TRANSM_SMALLER = 6
def transm_waypoints(voltage, transm, errors, lockpoint, triplets, t=0.1):
    # define transmission offset
    baseline = np.mean(transm)
    # define error signal offset
    error_baseline = np.mean(errors)
    # voltage tolerance
    vt = 0.01
    # relate lockpoint to line triplets
    lockpoint_voltage = voltage[lockpoint[0]]
    triplet_lock = None
    for i,triplet in enumerate(triplets):
        if triplet[3]-vt < lockpoint_voltage < triplet[3]+vt:
            triplet_lock = LEFT_SIDEBAND
            triplet_id = i
        if triplet[4]-vt < lockpoint_voltage < triplet[4]+vt:
            triplet_lock = CARRIER
            triplet_id = i
        if triplet[5]-vt < lockpoint_voltage < triplet[5]+vt:
            triplet_lock = RIGHT_SIDEBAND
            triplet_id = i
    if triplet_lock is None:
        print("Could not find triplet that lockpoint belongs to.")
        return
    # get transmission level
    ll = np.argmax(voltage>0.9*triplets[triplet_id][4]+0.1*triplets[triplet_id][3])
    ul = np.argmax(voltage>0.9*triplets[triplet_id][4]+0.1*triplets[triplet_id][5])
    transm_max = np.argmax(transm[ll:ul])+ll
    # direction of approach to the lockpoint is "increasing" for right sideband and "decreasing" for left
    direction = +1 if triplet_lock==RIGHT_SIDEBAND else -1
    # identify transmission maxima that need to be passed
    maxima = get_all_transmission_lines_larger(voltage, transm, transm_max, direction)
    # make waypoints:
    # starting point
    waypoints = deque()
    waypoints.append([GO_TO, -direction*200])
    waypoints.append([GO_UP if direction>0 else GO_DOWN, 0])
    # add 2 waypoints for each transmission maximum (except the last one)
    for maximum in maxima:
        level = transm[maximum]
        waypoints.append([TRANSM_LARGER, (1-t)*level+t*baseline])
        waypoints.append([TRANSM_SMALLER, 0.2*level+0.8*baseline])
    waypoints.pop()
    
    # now add the last waypoints on the error signal to the lockpoint
    flank = lockpoint[2]
    if triplet_lock!=CARRIER:
        # 1
        ll = min(maxima[-1], int((maxima[-1]+lockpoint[0])/2))
        ul = max(maxima[-1], int((maxima[-1]+lockpoint[0])/2))
        first_bump_level = (-direction*flank)*np.max((-direction*flank)*errors[ll:ul])
        waypoints.append([(-direction*flank), (1-t)*first_bump_level+t*error_baseline])
        # 2
        intermediate_level = errors[int((maxima[-1]+lockpoint[0])/2)]
        waypoints.append([direction*flank, t*first_bump_level+(1-t)*intermediate_level])
        # 3
        ll = min(lockpoint[0], int((maxima[-1]+lockpoint[0])/2))
        ul = max(lockpoint[0], int((maxima[-1]+lockpoint[0])/2))
        last_bump_level = (-direction*flank)*np.max((-direction*flank)*errors[ll:ul])
        waypoints.append([(-direction*flank), (1-t)*last_bump_level+t*intermediate_level])
        
    # start lock when error signal reaches offset level
    waypoints.append([flank*direction, lockpoint[1]])
    
    print("Waypoints for lockpoint @ %.2f:" % lockpoint_voltage)
    print(waypoints)
    return waypoints
    
    
def simulate_lock(voltage, transm, errors, waypoints):
    
    sim_volt = []
    sim_transm = []
    sim_errors = []
    i = 500000
    move = 0
    counter = 0
    plotpoints_transm = []
    plotpoints_errors = []
    while(len(waypoints)>0):
        sim_volt.append(voltage[i])
        sim_transm.append(transm[i])
        sim_errors.append(errors[i])
        if waypoints[0][0]==GO_TO:
            print("GO TO %.3f" % (waypoints[0][1]))
            i = v2i(waypoints[0][1])
            waypoints.popleft()
        elif waypoints[0][0]==GO_UP:
            print("GO UP @ %.2f" % (voltage[i]))
            move = +1
            waypoints.popleft()
        elif waypoints[0][0]==GO_DOWN:
            print("GO DOWN @ %.2f" % (voltage[i]))
            move = -1
            waypoints.popleft()
        elif waypoints[0][0]==ERRORS_LARGER and errors[i]>waypoints[0][1]:
            plotpoints_errors.append([counter,waypoints[0][1]])
            print("ERRORS LARGER %.3f @ %.2fV" % (waypoints[0][1],voltage[i]))
            waypoints.popleft()
        elif waypoints[0][0]==ERRORS_SMALLER and errors[i]<waypoints[0][1]:
            plotpoints_errors.append([counter,waypoints[0][1]])
            print("ERRORS SMALLER %.3f @ %.2fV" % (waypoints[0][1],voltage[i]))
            waypoints.popleft()
        elif waypoints[0][0]==TRANSM_LARGER and transm[i]>waypoints[0][1]:
            plotpoints_transm.append([counter,waypoints[0][1]])
            print("TRANSM LARGER %.3f @ %.2fV" % (waypoints[0][1],voltage[i]))
            waypoints.popleft()
        elif waypoints[0][0]==TRANSM_SMALLER and transm[i]<waypoints[0][1]:
            plotpoints_transm.append([counter, waypoints[0][1]])
            print("TRANSM SMALLER %.3f @ %.2fV" % (waypoints[0][1],voltage[i]))
            waypoints.popleft()
        i += move
        if i<0 or i>=1000000:
            print("ran out of range!")
            print(waypoints)
            return
        counter += 1
        
            
    fig, ax = plt.subplots(3, 2, figsize=(14,6))
    ax[0,0].plot(sim_volt, c='silver')
    ax[1,0].plot(sim_transm, c='silver')
    ax[2,0].plot(sim_errors, c='silver')
    ax[0,1].plot(sim_volt, c='silver')
    ax[1,1].plot(sim_transm, c='silver')
    ax[2,1].plot(sim_errors, c='silver')
    #ax[0,1].plot(np.arange(len(sim_volt)-10000, len(sim_volt)), sim_volt[-10000:], c='silver')
    #ax[1,1].plot(np.arange(len(sim_volt)-10000, len(sim_volt)), sim_transm[-10000:], c='silver')
    #ax[2,1].plot(np.arange(len(sim_volt)-10000, len(sim_volt)), sim_errors[-10000:], c='silver')
    ax[0,0].set_ylabel("Output (V)")
    ax[1,0].set_ylabel("Transm.")
    ax[2,0].set_ylabel("Err. Sig.")
    ax[2,0].set_xlabel("Steps")
    ax[2,1].set_xlabel("Steps")
    for point in plotpoints_transm:
        ax[1,0].scatter(point[0], point[1], c='r', s=15, zorder=20)
        ax[1,1].scatter(point[0], point[1], c='r', s=15, zorder=20)
    for point in plotpoints_errors:
        ax[2,0].scatter(point[0], point[1], c='r', s=15, zorder=20)
        ax[2,1].scatter(point[0], point[1], c='r', s=15, zorder=20)
    for i in range(3):
        ax[i,1].set_xlim(len(sim_volt)-10000, len(sim_volt)+500)
    plt.show()
    
    print("Final voltage is %.2f" % sim_volt[-1])
    return sim_volt[-1]



RELOAD = False
if RELOAD:
    data = np.genfromtxt("detailedtrace.txt")
voltage = np.linspace(-200,200,1000000)
transm = data[1]
errors = data[0]

fig, ax = plt.subplots(2,1, figsize=(25,8))
limits = [-40, 200]
ax[0].plot(voltage, transm)

ax[1].plot(voltage, errors)
ax[0].set_xlim(limits)
ax[1].set_xlim(limits)
plt.show()

lockpoints, flanks, triplets = lockpoints_function(voltage, transm, errors, -1, 0.1, 0.0)
print("Found %d lockpoints" % len(lockpoints))


results = np.zeros((len(lockpoints),2))
for index in range(len(lockpoints)):
    waypoints = transm_waypoints(voltage, transm, errors, lockpoints[index], triplets)
    print("Simulate waypoints. Final voltage should be %.2f" % voltage[lockpoints[index][0]])
    final_volt = simulate_lock(voltage, transm, errors, waypoints)
    print("\n")
    results[index] = [voltage[lockpoints[index][0]], final_volt]
    
print("Compare  target lockpoint with actual lockpoint from waypoints:")
print(results)