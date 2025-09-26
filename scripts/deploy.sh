#!/bin/bash
# scripts/deploy.sh

set -e

# Configuration
APP_NAME="streaming-engine"
INSTALL_DIR="/opt/$APP_NAME"
CONFIG_DIR="/etc/$APP_NAME"
LOG_DIR="/var/log/$APP_NAME"
DATA_DIR="/var/lib/$APP_NAME"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log() {
    echo -e "${GREEN}[$(date +'%Y-%m-%d %H:%M:%S')] $1${NC}"
}

warn() {
    echo -e "${YELLOW}[$(date +'%Y-%m-%d %H:%M:%S')] WARNING: $1${NC}"
}

error() {
    echo -e "${RED}[$(date +'%Y-%m-%d %H:%M:%S')] ERROR: $1${NC}"
    exit 1
}

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   error "This script must be run as root"
fi

# Create directories
log "Creating directories..."
mkdir -p $INSTALL_DIR $CONFIG_DIR $LOG_DIR $DATA_DIR

# Copy application files
log "Copying application files..."
cp -r bin/ $INSTALL_DIR/
cp -r config/* $CONFIG_DIR/
cp systemd/*.service /etc/systemd/system/

# Create streaming user
if ! id "streaming" &>/dev/null; then
    log "Creating streaming user..."
    useradd -r -s /bin/false -d $INSTALL_DIR streaming
fi

# Set permissions
log "Setting permissions..."
chown -R streaming:streaming $INSTALL_DIR $LOG_DIR $DATA_DIR
chmod 755 $INSTALL_DIR/bin/*
chmod 644 $CONFIG_DIR/*

# Enable and start service
log "Enabling systemd service..."
systemctl daemon-reload
systemctl enable streaming-server.service

log "Starting streaming server..."
systemctl start streaming-server.service

# Wait for service to start
sleep 5

# Check service status
if systemctl is-active --quiet streaming-server.service; then
    log "Streaming server started successfully"
else
    error "Failed to start streaming server"
    systemctl status streaming-server.service
fi

log "Deployment completed successfully!"