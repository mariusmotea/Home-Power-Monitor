<?php
require_once('config.php');

$values = array();
$sql        = "SELECT DATE_FORMAT(time, '%m/%d %H:%i'), senz1, senz2, senz3, senz4, senz5, senz6, senz7, senz8  FROM data WHERE time BETWEEN NOW() - INTERVAL 30 DAY AND NOW();";
$res        = mysqli_query($connection, $sql);
while ($row = mysqli_fetch_assoc($res)) {
  $values[] = $row;
}
//print_r($values);
 ?>
 <!DOCTYPE html>
 <html style="height: 100%">
    <head>
        <meta charset="utf-8">
    </head>
    <body style="height: 100%; margin: 0">
        <div id="container" style="height: 100%"></div>
        <script type="text/javascript" src="echarts.min.js"></script>
        <script type="text/javascript">
 var dom = document.getElementById("container");
 var myChart = echarts.init(dom);
 option = null;

 var timeData = [
<?php
foreach ($values as $key) {
  echo "'"  . $key["DATE_FORMAT(time, '%m/%d %H:%i')"] .  "', " ;
}
?>   ];


 option = {
     title: {
         text: 'Powermeter',
         subtext: 'Sensors data per fuse',
         x: 'center'
     },
     tooltip: {
         trigger: 'axis',
         axisPointer: {
             animation: false
         }
     },
     legend: {
         data:['senz1','senz2','senz3','senz4','senz5','senz6','senz7','total'],
         x: 'left'
     },
     toolbox: {
         feature: {
             dataZoom: {
                 yAxisIndex: 'none'
             },
             restore: {},
             saveAsImage: {}
         }
     },
     axisPointer: {
         link: {xAxisIndex: 'all'}
     },
     dataZoom: [
         {
             show: true,
             realtime: true,
             start: 70,
             end: 1000,
             xAxisIndex: [0, 1]
         },
         {
             type: 'inside',
             realtime: true,
             start: 70,
             end: 100,
             xAxisIndex: [0, 1]
         }
     ],
     grid: [{
         left: 50,
         right: 50,
         height: '35%'
     }, {
         left: 50,
         right: 50,
         top: '55%',
         height: '35%'
     }],
     xAxis : [
         {
             type : 'category',
             boundaryGap : false,
             axisLine: {onZero: true},
             data: timeData
         },
         {
             gridIndex: 1,
             type : 'category',
             boundaryGap : false,
             axisLine: {onZero: true},
             data: timeData
             //position: 'top'
         }
     ],
     yAxis : [
         {
             name : 'senz1(W)',
             type : 'value',
             max : 500
         },
         {
             gridIndex: 1,
             name : 'total(W)',
             type : 'value',
             inverse: false
         }
     ],
     series : [

<?php
for ($x = 1; $x <= 7; $x++) {
?>
{
    name:'senz<?php print $x;?>',
    type:'line',
    smooth: true,
    symbolSize: 8,
    hoverAnimation: false,
    data:[
      <?php
      foreach ($values as $key) {
        echo $key["senz" . $x] .  "," ;
      }
      ?>
    ],
   markPoint : {
       data : [
           {type : 'max', name: 'Max'},
           {type : 'min', name: 'Min'}
       ]
   },
   markLine : {
       data : [
           {type : 'average', name: 'Average'}
       ]
   }
},
<?php
}
?>

         {
             name:'total',
             type:'line',
             smooth: true,
             xAxisIndex: 1,
             yAxisIndex: 1,
             symbolSize: 8,
             hoverAnimation: false,
             data: [
               <?php
               foreach ($values as $key) {
                 echo $key["senz8"] .  "," ;
               }
               ?>
             ],
             markPoint : {
                 data : [
                     {type : 'max', name: 'Max'},
                     {type : 'min', name: 'Min'}
                 ]
             },
             markLine : {
                 data : [
                     {type : 'average', name: 'Average'}
                 ]
             }
         }
     ]
 };
 if (option && typeof option === "object") {
     myChart.setOption(option, true);
 }
        </script>
    </body>
 </html>
