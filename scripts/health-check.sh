#!/bin/bash
# scripts/health-check.sh

# Health check for streaming server

# Configuration
HEALTH_URL="http://localhost:8080/health"
METRICS_URL="http://localhost:9090/metrics"
TIMEOUT=5

# Check HTTP server
if curl -s -f --max-time $TIMEOUT $HEALTH_URL > /dev/null; then
    echo "HTTP server is healthy"
else
    echo "HTTP server is not responding"
    exit 1
fi

# Check metrics endpoint
if curl -s -f --max-time $TIMEOUT $METRICS_URL > /dev/null; then
    echo "Metrics endpoint is healthy"
else
    echo "Metrics endpoint is not responding"
    exit 1
fi

# Check disk space
DISK_USAGE=$(df / | awk 'NR==2 {print $5}' | sed 's/%//')
if [ $DISK_USAGE -gt 90 ]; then
    echo "Disk usage is too high: $DISK_USAGE%"
    exit 1
fi

# Check memory usage
MEMORY_USAGE=$(free | awk 'NR==2{printf "%.0f", $3*100/$2}')
if [ $MEMORY_USAGE -gt 85 ]; then
    echo "Memory usage is too high: $MEMORY_USAGE%"
    exit 1
fi

echo "All health checks passed"
exit 0