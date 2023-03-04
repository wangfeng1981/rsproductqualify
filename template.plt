#设置输出终端 输出到png文件
set terminal png size 1024,1024  font "simsun.ttc,12"
#输出文件名  
set output '{{{outscatterpngfile}}}'

# 设置数据文件分割值
set datafile separator ' '
# 设置俯视图
set view map scale 1
# 设置缺失值
set datafile missing '0'

#设置X轴范围 
set xrange [ {{{xrange0}}}: {{{xrange1}}} ]   
#设置Y轴范围  
set yrange [ {{{yrange0}}}: {{{yrange1}}} ]  

#设置渐变色模板，
#模板名称查看http://gnuplot.info/docs_5.5/loc13535.html
#rgbformulae 33,13,10 为rainbow，从蓝色到绿色到红色
set palette rgbformulae 33,13,10
#设置颜色轴为对数尺度
set log cb

#  
set xlabel "{{{xlabel}}}"
set ylabel "{{{ylabel}}}"

# 拟合方程  
set label '拟合公式: {{{fitequation}}}' at screen 0.13, 0.86 front
# 相关系数 
set label '相关系数: {{{corr}}}' at screen 0.13, 0.84 front
# R2
set label 'R平方: {{{r2}}}' at screen 0.13, 0.82 front
# 样本数量 
set label '样本数量: {{{samplecount}}}' at screen 0.13, 0.80 front

# 绘制3D视图，第三位使用颜色表示 
# 密度数据文件              
# 拟合曲线文件，两个点x,y,z  
# https://docs.w3cub.com/gnuplot/linetypes_colors_styles.html 讲解不错
#lt 是线色 , dt dashtype点划线类型 , lw 线宽
splot '{{{heatdatafile}}}' using 1:2:3 with pm3d title '', '{{{fitlinedatafile}}}' using 1:2:3 with lines title '' lc rgb '#000000' dt solid lw 2 



#########  Graph II ##########
reset
set terminal png size 1024,1024
#输出文件名 
set output '{{{outhistpngfile}}}'
set datafile separator ' '

#  
set xlabel "{{{histxlabel}}}"
set ylabel "{{{histylabel}}}"

# 
set xrange [{{{histXmin}}}:{{{histXmax}}}]
set yrange [0.0:{{{histYmax}}}]

#vertical line of zero-x
$linezerox << EOD
0.0 0.0
0.0 {{{histYmax}}}
EOD

# 
plot '{{{histdatafile}}}' using 1:2 title '' with boxes , '$linezerox' using 1:2 title '' with lines dt 3 lc rgb '#FF0000' lw 1