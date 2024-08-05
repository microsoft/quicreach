
// The latest raw data (to be read in from the data.csv file)
var reachData = []

// Fixed charting values
var dataMaxCount = 1200
var dataLineWidth = 2
var dataRawPointRadius = 0

function generateUnreachabilityDataset() {
    var output = []
    reachData.forEach(
        p => output.push({x:new Date(p.UtcDateTime), y:p.Total - p.Reachable}))
    return output
}

function generateReachabilityDataset() {
    var output = []
    reachData.forEach(
        p => output.push({x:new Date(p.UtcDateTime), y:p.Reachable}))
    return output
}

function generateGoodReachableDataset() {
    var output = []
    reachData.forEach(
        p => output.push({x:new Date(p.UtcDateTime), y:p.Reachable - p.TooMuch - p.MultiRtt}))
    return output
}

function generateTooMuchDataset() {
    var output = []
    reachData.forEach(
        p => output.push({x:new Date(p.UtcDateTime), y:p.TooMuch}))
    return output
}

function generateMultiRttDataset() {
    var output = []
    reachData.forEach(
        p => output.push({x:new Date(p.UtcDateTime), y:p.MultiRtt}))
    return output
}

function generateQuicV2Dataset() {
    var output = []
    reachData.forEach(
        p => output.push({x:new Date(p.UtcDateTime), y:p.QuicV2}))
    return output
}

function createDataset(name, data, fillColor) {
    return {
        type: "line",
        label: name,
        borderWidth: dataLineWidth,
        pointRadius: dataRawPointRadius,
        tension: 0,
        data: data.slice(-dataMaxCount),
        backgroundColor: fillColor || "rgba(0, 0, 255, 0.5)",
    };
}

function titlePlacement(tooltipItem, data) {
    var dataset = data.datasets[tooltipItem[0].datasetIndex]
    var datapoint = dataset.data[tooltipItem[0].index]
    return new Date(datapoint.x).toString()
}

function createChartwithData(elementId, dataset, displayLegend, stacked) {
    new Chart(document.getElementById(elementId).getContext('2d'), {
        data: { datasets: dataset },
        options: {
            maintainAspectRatio: false,
            scales: {
                xAxes: [{
                    type: 'linear',
                    gridLines: {
                      display: false,
                      drawBorder: false
                    },
                    ticks: {
                        callback: function(value) {
                            return new Date(value).toDateString()
                        }
                    }
                }],
                yAxes: [{
                    stacked: stacked,
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
              display: displayLegend
            },
            tooltips: {
                backgroundColor: "rgb(255,255,255)",
                bodyFontColor: "#858796",
                titleMarginBottom: 10,
                titleFontColor: '#6e707e',
                titleFontSize: 14,
                borderColor: '#dddfeb',
                borderWidth: 1,
                mode: "nearest",
                intersect: false,
                callbacks : {
                    title: titlePlacement
                }
            }
        }
    })
}

function createChart() {
    var reachable = []
    reachable.push(createDataset("Reachable", generateReachabilityDataset()))
    createChartwithData("canvasReachable", reachable, false, false)

    var breakdown = []
    breakdown.push(createDataset("Reachable", generateGoodReachableDataset(), "rgba(0, 255, 0, 0.5)"))
    breakdown.push(createDataset("Reachable (Too Much)", generateTooMuchDataset(), "rgba(255, 0, 0, 0.5)"))
    breakdown.push(createDataset("Reachable (Multi RTT)", generateMultiRttDataset(), "rgba(0, 0, 255, 0.5)"))
    createChartwithData("canvasReachBreakdown", breakdown, true, true)

    var quicv2 = []
    quicv2.push(createDataset("Version 2", generateQuicV2Dataset()))
    createChartwithData("canvasQuicV2", quicv2, false, true)
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
