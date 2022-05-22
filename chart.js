

// The latest raw data (to be read in from the data.csv file)
var reachData = []

// Fixed charting values
var dataMaxCount = 50
var dataLineWidth = 2
var dataRawPointRadius = 3

function generateReachabilityDataset() {
    var output = []
    reachData.forEach(
        p => output.push({x:new Date(p.UtcDateTime), y:p.Reachable}))
    return output
}

function createDataset() {
    return {
        type: "line",
        label: "Reachable",
        backgroundColor: "#11a718",
        borderColor: "#11a718",
        borderWidth: dataLineWidth,
        pointRadius: dataRawPointRadius,
        tension: 0,
        data: generateReachabilityDataset(),
        fill: false,
        sortOrder: 1,
        hidden: false,
        hiddenType: false
    };
}

function createChart() {
    var datasets = []
    datasets.push(createDataset())

    new Chart(document.getElementById("canvasReachable").getContext('2d'), {
        data: { datasets: datasets},
        options: {
            maintainAspectRatio: false,
            scales: {
                xAxes: [{
                    type: 'linear',
                    offset: true,
                    gridLines: {
                      display: false,
                      drawBorder: false
                    }
                }],
                yAxes: [{
                    gridLines: {
                        color: "rgb(234, 236, 244)",
                        zeroLineColor: "rgb(234, 236, 244)",
                        drawBorder: false,
                        borderDash: [2],
                        zeroLineBorderDash: [2]
                    }
                }]
            },
            legend: {
              display: false
            },
            tooltips: {
                backgroundColor: "rgb(255,255,255)",
                bodyFontColor: "#858796",
                titleMarginBottom: 10,
                titleFontColor: '#6e707e',
                titleFontSize: 14,
                borderColor: '#dddfeb',
                borderWidth: 1,
                xPadding: 15,
                yPadding: 15,
                mode: "nearest",
                intersect: false
            }
        }
    })
}

function processSearchParams() {
    var url = new URL(window.location.href);
    var param = url.searchParams.get("count")
    if (param) {
        dataMaxCount = param
    }
    var param = url.searchParams.get("width")
    if (param) {
        dataLineWidth = param
    }
    var param = url.searchParams.get("radius")
    if (param) {
        dataRawPointRadius = param
    }
}

function processData(allText) {
    var allTextLines = allText.split(/\r\n|\n/);
    var headers = allTextLines[0].split(',');
    for (var i=1; i<allTextLines.length; i++) {
        var data = allTextLines[i].split(',');
        if (data.length == headers.length) {
            var tarr = [];
            for (var j=0; j<headers.length; j++) {
                tarr[headers[j]] = data[j]
            }
            reachData.push(tarr);
        }
    }
}

window.onload = function() {
    // Read in the latest data from GitHub.
    fetch('https://raw.githubusercontent.com/microsoft/quicreach/data/data.csv')
        .then(response => response.text())
        .then((data) => {
            processData(data)
            processSearchParams()
            createChart()
        })
};
