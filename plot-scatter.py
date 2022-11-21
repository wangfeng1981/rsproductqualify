#-------------------------------------------------------------------------------
# Name:        散点图，直方图
# Purpose:
#
# Author:      wangfeng
# 
# Created:     14/11/2022
# Copyright:   (c) edz 2022
# Licence:     <your licence>
#
# version 1.0.0 2022-11-21
#-------------------------------------------------------------------------------
import sys
import json
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import colors


from scipy.stats import norm
from scipy import spatial, stats
from scipy import optimize
from scipy.stats import gaussian_kde


def Draw_hist(xdata,xmin,xmax,nbins,xlabel,ylabel,title,outfilename):
    fig,axes = plt.subplots()
    freq,bins,_=axes.hist(xdata,bins=nbins,range=[xmin,xmax])
    axes.grid(alpha=0.5)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    #plt.show()
    plt.savefig(outfilename)
    return 0


def Draw_scatter_with_density(xdata,ydata,title,xlabel,ylabel,xmin,xmax,ymin,ymax,outputfilename):

    #regression coefficients r_value1相关系数
    slope1, intercept1, r_value1, p_value1, std_err1 = stats.linregress(xdata, ydata)
    reg_equation='y='+str(round(slope1,4))+'x + '+str(round(intercept1,4))
    reg_relcorr ='corr=' + str(round(r_value1,4)) #相关系数


    #fit line
    fit_result = np.polyfit(xdata,ydata,1)
    print(fit_result)
    x_reg = np.array([-9999,9999])
    y_reg = fit_result[0]*x_reg + fit_result[1]

    #作图
    fig, ax = plt.subplots(figsize=(8, 6), dpi=300)
    H = ax.hist2d(xdata, ydata, bins=(200,200),cmap=plt.get_cmap('jet'),norm=colors.LogNorm(),zorder=2) #draw scatters with density
    ax.plot(x_reg,y_reg,color='#000',alpha=0.5) #draw regression line
    clb=fig.colorbar(H[3], ax=ax)

    ax.set_xlim(xmin,xmax)
    ax.set_ylim(ymin,ymax)

    ax.set_xlabel(xlabel, fontsize=12)
    ax.set_ylabel(ylabel, fontsize=12)
    ax.grid(alpha=0.5)
    plt.title(title)

    #绘制拟合公式和相关系数
    ax.text(0.05,0.85, reg_equation)
    ax.text(0.05,0.80, reg_relcorr)

    #plt.show()
    plt.savefig(outputfilename)

    return 0


def main():
    if len(sys.argv) == 1:
        print("no enough params. usage: python thescript.py order.json")
        return 1
    orderJsonfile = sys.argv[1]
    print("working for " + orderJsonfile)
    jsonfilelun = open(orderJsonfile)
    jsondata = json.load(jsonfilelun)

    filename1=jsondata['indatarawfilename']
    filename2=jsondata['refdatarawfilename']
    x = np.fromfile(filename1, dtype=np.float32);
    y = np.fromfile(filename2, dtype=np.float32);

    title1=jsondata['inTag']+" VS "+jsondata['reTag'] + " " + jsondata['matcher']

    Draw_scatter_with_density(x,y,title1, jsondata['xlabel'] , jsondata['ylabel'],jsondata['scatterXmin'],jsondata['scatterXmax'],jsondata['scatterYmin'],jsondata['scatterYmax'], filename1+'-sca.png')

    filename3 = jsondata['diffrawfilename']
    filename4 = jsondata['reldiffrawfilename']
    histdata1 = np.fromfile(filename3, dtype=np.float32);
    histdata2 = np.fromfile(filename4, dtype=np.float32);
    xlabel=jsondata['histXLabel']
    rxlabel=jsondata['rehistXLabel']
    Draw_hist(histdata1, jsondata['histXmin'] , jsondata['histXmax'] , jsondata['histCount'] ,xlabel, jsondata['histYLabel'],title1,filename3+'.png')
    Draw_hist(histdata2, jsondata['histXminRE'] , jsondata['histXmaxRE'] , jsondata['histCount'] ,rxlabel, jsondata['rehistYLabel'],title1,filename4+'.png')
    return 0


#测试函数
def Test():
    return 0

if __name__ == '__main__':
    main()
