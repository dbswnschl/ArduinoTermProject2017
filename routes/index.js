var express = require('express');
var bodyParser = require('body-parser');
var mysql = require('mysql');
var app = express.Router();
app.use(bodyParser.urlencoded({ extended: false }));

var connection = mysql.createConnection({
  host: 'localhost',
  user: 'root',
  password: 'root',
  database: 'arduino',
  port: '3308'
});

connection.connect();
connection.query('SELECT * FROM `light_sensor`', function (err, rows, fields) {
  if (err) {
    console.log(err);
  }
  console.log('*SQL서버 연동완료*');

});

var gps = {
  lat: 0,
  lng: 0
};
function getgps() {
  return gps;
}

/* GET home page. */
app.get('/', function (req, res, next) {
  res.render('index', { title: 'Express' });
});

var printErr = function (msg, err) {
  console.log("[[에러발생]] " + msg);
  console.log(err);
}

app.post('/', function (req, res, next) {
  var result = req.body;

  if (result != null) {
    var temp = result.TEMP;
    var light = result.LIGHT;
    var humidity = result.HUMIDITY;
    gps = result.GPS;

    connection.query(`INSERT INTO light_sensor (value) VALUES ('${light}');`, function (err_light, row_light, field_light) {
      if (err_light) {
        printErr("QUERY INSERT at light_sensor", err_light);
      } else {
        console.log("light_sensor값 전송 완료");
        connection.query(`INSERT INTO temp_sensor (value) VALUES ('${temp}');`, function (err_temp, row_temp, field_temp) {
          if (err_temp) {
            printErr("QUERY INSERT at temp_sensor", err_temp);
          } else {
            console.log("temp_sensor값 전송 완료");
            connection.query(`INSERT INTO humidity_sensor (value) VALUES ('${temp}');`, function (err_humidity, row_humidity, field_humidity) {
              if (err_humidity) {
                printErr("QUERY INSERT at humidity_sensor", err_humidity);
              } else {
                console.log("humidity_sensor값 전송 완료");
                if (gps.lat == 0 || gps.lng == 0) {
                  console.log("gps값이 올바르지 않으므로 전송하지 않습니다.");

                } else
                  connection.query(`INSERT INTO gps_sensor (lat,lng) VALUES ('${gps.lat}','${gps.lng}');`, function (err_gps, row_gps, field_gps) {
                    if (err_gps) {
                      printErr("QUERY INSERT at gps_sensor", err_gps);
                    } else {

                      console.log("gps_sensor값 전송 완료");

                    }
                  });
              }
            });
          }
        });
      }
    });
  } else {
    result = "error";
  }
  res.send(result);
});
app.get('/client', function (req, res) {
  connection.query(`SELECT * FROM gps_sensor ORDER BY id desc limit 1; `, function (err, row, field) {
    connection.query(`SELECT * FROM light_sensor ORDER BY id desc limit 1;`, function (err1, row1, field1) {
      connection.query(`SELECT * FROM temp_sensor ORDER BY id desc limit 1;`, function (err2, row2, field2) {
        connection.query(`SELECT * FROM humidity_sensor ORDER BY id desc limit 1;`, function (err3, row3, field3) {
          if (err || err1 || err2 || err3) {
            console.log("ERROR");
            res.send("ERROR");
          } else {
            temp = { val: row2[0].value, times: row2[0].times };
            light = { val: row1[0].value, times: row1[0].times };
            humidity = { val: row3[0].value, times: row3[0].times };
            var mygps = { lat: row[0].lat, lng: row[0].lng };
            res.render('client', { mygps: mygps, gpstime: row[0].times, mytemp: temp, mylight: light, myhumidity: humidity });
          }
        });
      });
    });
  });
});








module.exports = app;
