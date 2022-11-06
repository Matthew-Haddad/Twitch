import os
import numpy as np
import pandas as pd
import torch
import math
import torch.nn as nn
import torch.nn.functional as F
import matplotlib.pyplot as plt
import torch.optim as optim
import torch.distributions as distributions
from torch import optim
from torch.utils.data import DataLoader
from torchvision import datasets, transforms

def get_ds_infos():
    ## 0:Code, 1:Weight, 2:Height, 3:Age, 4:Gender
    dss = np.genfromtxt("motionSense/data_subjects_info.csv",delimiter=',')
    dss = dss[1:]
    print("----> Data subjects information is imported.")
    return dss

def create_time_series(num_features, num_act_labels, num_gen_labels, label_codes, trial_codes):
    dataset_columns = num_features+num_act_labels+num_gen_labels
    ds_list = get_ds_infos()
    train_data = np.zeros((0,dataset_columns))
    test_data = np.zeros((0,dataset_columns))
    for i, sub_id in enumerate(ds_list[:,0]):
        for j, act in enumerate(label_codes):
            for trial in trial_codes[act]:
                fname = 'motionSense/A_DeviceMotion_data/A_DeviceMotion_data/'+act+'_'+str(trial)+'/sub_'+str(int(sub_id))+'.csv'
                raw_data = pd.read_csv(fname)
                raw_data = raw_data.drop(['Unnamed: 0'], axis=1)
                unlabel_data = raw_data.values
                label_data = np.zeros((len(unlabel_data), dataset_columns))
                label_data[:,:-(num_act_labels + num_gen_labels)] = unlabel_data
                label_data[:,label_codes[act]] = 1
                label_data[:,-(num_gen_labels)] = int(ds_list[i,4])
                ## We consider long trials as training dataset and short trials as test dataset
                if trial > 10:
                    test_data = np.append(test_data, label_data, axis = 0)
                else:
                    train_data = np.append(train_data, label_data, axis = 0)
    return train_data , test_data

def getMotionSenseData():
    ## Here we set parameter to build labeld time-series from dataset of "(A)DeviceMotion_data"
    num_features = 12 # attitude(roll, pitch, yaw); gravity(x, y, z); rotationRate(x, y, z); userAcceleration(x,y,z)
    num_act_labels = 6 # dws, ups, wlk, jog, sit, std
    num_gen_labels = 1 # 0/1(female/male)
    label_codes = {"dws":num_features, "ups":num_features+1, "wlk":num_features+2, "jog":num_features+3, "sit":num_features+4, "std":num_features+5}
    trial_codes = {"dws":[1,2,11], "ups":[3,4,12], "wlk":[7,8,15], "jog":[9,16], "sit":[5,13], "std":[6,14]}
    ## Calling 'create_time_series()' to build time-series
    print("--> Building Training and Test Datasets...")
    train_ts, test_ts = create_time_series(num_features, num_act_labels, num_gen_labels, label_codes, trial_codes)
    print("--> Shape of Training Time-Series:", train_ts.shape)
    print("--> Shape of Test Time-Series:", test_ts.shape)
    return train_ts, test_ts

def plot(xs, ys, xlim=(-3, 3), ylim=(-1, 12)):
  fig, ax = plt.subplots()
  ax.plot(xs, ys, linewidth=5)
  # ax.set_aspect('equal')
  ax.grid(True, which='both')

  ax.axhline(y=0, color='k')
  ax.axvline(x=0, color='k')
  ax.set_xlim(*xlim)
  ax.set_ylim(*ylim)

def get_tensors(dataset, num_channels=12, num_frames=5, num_labels=6):
    input_sz = num_frames*num_channels
    x = torch.tensor(dataset[:,:num_channels]).float()
    y = torch.tensor(dataset[:,num_channels:num_channels + num_labels]).float()
    classes, data, labels, new_data = [], [], [], []
    for i in range(6):
        new_class = dataset[np.where(dataset[:,num_channels + i])]
        classes.append(new_class)
        data.append(classes[i][:,:num_channels])
        labels.append(classes[i][:,num_channels + 1:])
        # crop & reshape data
        data[i] = data[i][:data[i].shape[0] - data[i].shape[0] % num_frames,:]
        new_data.append(data[i].reshape(-1,input_sz).astype(float))
        # optional: format as regression
        new_data[i] = np.array([np.append(new_data[i][j],i) for j in range(len(new_data[i]))])
    arr = np.array(np.concatenate(new_data, axis=0))
    x = arr[:,:-1]
    y = arr[:,-1:]
    x = torch.tensor(x).float()
    y_target = torch.tensor(y).float()
    print("Shape of data: ", x.shape)
    print("Shape of labels: ", y.shape)
    return x, y_target
