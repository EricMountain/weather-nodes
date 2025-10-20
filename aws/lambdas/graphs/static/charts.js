// Initialize default date range (last 24 hours)
const now = new Date();
const yesterday = new Date(now.getTime() - 24 * 60 * 60 * 1000);

document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('end-date').value = formatDateForInput(now);
    document.getElementById('start-date').value = formatDateForInput(yesterday);
});

// Chart dimensions
const margin = {top: 20, right: 80, bottom: 50, left: 60};
let width = document.getElementById('chart').offsetWidth - margin.left - margin.right;
const height = 500 - margin.top - margin.bottom;

// Color scale for different devices
const colorScale = d3.scaleOrdinal(d3.schemeCategory10);

function formatDateForInput(date) {
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    const hours = String(date.getHours()).padStart(2, '0');
    const minutes = String(date.getMinutes()).padStart(2, '0');
    return `${year}-${month}-${day}T${hours}:${minutes}`;
}

async function generateGraph() {
    const startDate = document.getElementById('start-date').value;
    const endDate = document.getElementById('end-date').value;
    const metric = document.getElementById('metric').value;
    
    if (!startDate || !endDate) {
        alert('Please select both start and end dates');
        return;
    }
    
    const selectedDevices = Array.from(document.querySelectorAll('input[name="device"]:checked'))
        .map(cb => cb.value);
    
    if (selectedDevices.length === 0) {
        alert('Please select at least one device');
        return;
    }
    
    // Show loading state
    document.getElementById('chart').innerHTML = '<div class="loading">Loading data...</div>';
    document.getElementById('generate-graph').disabled = true;
    
    try {
        const response = await fetch(window.location.href, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
                'X-API-Key': getApiKeyFromUrl()
            },
            body: new URLSearchParams({
                start_date: new Date(startDate).toISOString(),
                end_date: new Date(endDate).toISOString(),
                metric: metric,
                devices: selectedDevices
            })
        });
        
        const data = await response.json();
        
        if (data.success) {
            drawChart(data);
        } else {
            showError('Error: ' + (data.error || 'Unknown error'));
        }
        
    } catch (error) {
        showError('Error fetching data: ' + error.message);
    } finally {
        document.getElementById('generate-graph').disabled = false;
    }
}

function drawChart(data) {
    // Clear previous chart
    d3.select('#chart').selectAll('*').remove();
    
    // Prepare data
    const allDataPoints = [];
    const deviceNames = [];
    
    Object.keys(data.data).forEach(deviceId => {
        const deviceData = data.data[deviceId];
        deviceData.forEach(point => {
            allDataPoints.push({
                ...point,
                deviceId: deviceId,
                date: new Date(point.timestamp)
            });
        });
        deviceNames.push(deviceId);
    });
    
    if (allDataPoints.length === 0) {
        document.getElementById('chart').innerHTML = '<div class="loading">No data found for the selected criteria</div>';
        return;
    }
    
    // Create SVG
    const svg = d3.select('#chart')
        .append('svg')
        .attr('width', width + margin.left + margin.right)
        .attr('height', height + margin.top + margin.bottom);
    
    const g = svg.append('g')
        .attr('transform', `translate(${margin.left},${margin.top})`);
    
    // Scales
    const xScale = d3.scaleTime()
        .domain(d3.extent(allDataPoints, d => d.date))
        .range([0, width]);
    
    const yScale = d3.scaleLinear()
        .domain(d3.extent(allDataPoints, d => d.value))
        .nice()
        .range([height, 0]);
    
    // Line generator
    const line = d3.line()
        .x(d => xScale(d.date))
        .y(d => yScale(d.value))
        .curve(d3.curveLinear);
    
    // Add axes
    g.append('g')
        .attr('class', 'axis')
        .attr('transform', `translate(0,${height})`)
        .call(d3.axisBottom(xScale)
            .tickFormat(d3.timeFormat('%m/%d %H:%M')));
    
    g.append('g')
        .attr('class', 'axis')
        .call(d3.axisLeft(yScale));
    
    // Add axis labels
    g.append('text')
        .attr('transform', 'rotate(-90)')
        .attr('y', 0 - margin.left)
        .attr('x', 0 - (height / 2))
        .attr('dy', '1em')
        .style('text-anchor', 'middle')
        .text(getMetricLabel(data.metric));
    
    g.append('text')
        .attr('transform', `translate(${width / 2}, ${height + margin.bottom - 10})`)
        .style('text-anchor', 'middle')
        .text('Time');
    
    // Add tooltip
    const tooltip = d3.select('body').append('div')
        .attr('class', 'tooltip')
        .style('opacity', 0);
    
    // Draw lines for each device
    deviceNames.forEach((deviceId, i) => {
        const deviceData = allDataPoints.filter(d => d.deviceId === deviceId);
        if (deviceData.length === 0) return;
        
        // Group by device_name within each device
        const groupedData = d3.group(deviceData, d => d.device_name);
        
        groupedData.forEach((points, deviceName) => {
            const color = colorScale(`${deviceId}-${deviceName}`);
            
            // Add line
            g.append('path')
                .datum(points)
                .attr('class', 'line')
                .attr('d', line)
                .style('stroke', color);
            
            // Add dots
            g.selectAll(`.dot-${deviceId}-${deviceName}`)
                .data(points)
                .enter().append('circle')
                .attr('class', `dot dot-${deviceId}-${deviceName}`)
                .attr('cx', d => xScale(d.date))
                .attr('cy', d => yScale(d.value))
                .attr('r', 1)
                .style('fill', color)
                .on('mouseover', function(event, d) {
                    tooltip.transition()
                        .duration(200)
                        .style('opacity', .9);
                    tooltip.html(`Device: ${deviceId}<br/>Sensor: ${d.device_name}<br/>Value: ${d.value}<br/>Time: ${d.date.toLocaleString()}`)
                        .style('left', (event.pageX + 10) + 'px')
                        .style('top', (event.pageY - 28) + 'px');
                })
                .on('mouseout', function(d) {
                    tooltip.transition()
                        .duration(500)
                        .style('opacity', 0);
                });
        });
    });
    
    // Add legend
    const legend = g.append('g')
        .attr('class', 'legend')
        .attr('transform', `translate(10, 0)`);
    
    let legendY = 0;
    deviceNames.forEach(deviceId => {
        const deviceData = allDataPoints.filter(d => d.deviceId === deviceId);
        const deviceNames = [...new Set(deviceData.map(d => d.device_name))];
        
        deviceNames.forEach(deviceName => {
            const color = colorScale(`${deviceId}-${deviceName}`);
            
            legend.append('rect')
                .attr('x', 0)
                .attr('y', legendY)
                .attr('width', 12)
                .attr('height', 12)
                .style('fill', color);
            
            legend.append('text')
                .attr('x', 16)
                .attr('y', legendY + 6)
                .attr('dy', '0.35em')
                .text(`${deviceId} - ${deviceName}`);
            
            legendY += 20;
        });
    });
}

function getMetricLabel(metric) {
    const labels = {
        'temperature': 'Temperature (°C)',
        'humidity': 'Humidity (%)',
        'pressure': 'Pressure (hPa)',
        'battery': 'Battery Voltage (V)',
        'wifi_dbm': 'WiFi Signal (dBm)',
        'free_heap_bytes': 'Free Heap (bytes)'
    };
    return labels[metric] || metric;
}

function showError(message) {
    document.getElementById('chart').innerHTML = `<div class="error">${message}</div>`;
}

function getApiKeyFromUrl() {
    // Extract API key from URL parameters if present
    const urlParams = new URLSearchParams(window.location.search);
    return urlParams.get('api_key') || '';
}

// Update the width dynamically on window resize
document.addEventListener('DOMContentLoaded', () => {
    const updateWidth = () => {
        width = document.getElementById('chart').offsetWidth - margin.left - margin.right;
        // Update the chart dimensions dynamically here if needed
    };
    window.addEventListener('resize', updateWidth);
    updateWidth(); // Initial call to set the width
});