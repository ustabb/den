#!/bin/bash
# cloud/scripts/deploy-cloud.sh

set -e

# Configuration
CLUSTER_NAME="streaming-cluster"
REGION="us-west-2"
ENVIRONMENT="production"
DOCKER_IMAGE="registry.gitlab.com/your-org/streaming-engine:latest"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log() { echo -e "${GREEN}[$(date +'%Y-%m-%d %H:%M:%S')] $1${NC}"; }
warn() { echo -e "${YELLOW}[$(date +'%Y-%m-%d %H:%M:%S')] WARNING: $1${NC}"; }
error() { echo -e "${RED}[$(date +'%Y-%m-%d %H:%M:%S')] ERROR: $1${NC}"; exit 1; }

# Check prerequisites
check_prerequisites() {
    log "Checking prerequisites..."
    
    command -v terraform >/dev/null 2>&1 || error "Terraform not installed"
    command -v aws >/dev/null 2>&1 || error "AWS CLI not installed"
    command -v kubectl >/dev/null 2>&1 || error "kubectl not installed"
    command -v docker >/dev/null 2>&1 || error "Docker not installed"
    
    aws sts get-caller-identity >/dev/null 2>&1 || error "AWS not configured"
    
    log "All prerequisites satisfied"
}

build_and_push_image() {
    log "Building and pushing Docker image..."
    
    docker build -f cloud/docker/Dockerfile.cloud -t $DOCKER_IMAGE .
    docker push $DOCKER_IMAGE
    
    log "Docker image pushed successfully"
}

deploy_infrastructure() {
    log "Deploying cloud infrastructure with Terraform..."
    
    cd cloud/terraform/aws
    
    terraform init -upgrade
    terraform plan -var="environment=$ENVIRONMENT" -out=tfplan
    terraform apply -auto-approve tfplan
    
    # Update kubeconfig
    aws eks update-kubeconfig --region $REGION --name $CLUSTER_NAME
    
    cd - > /dev/null
    
    log "Infrastructure deployed successfully"
}

deploy_kubernetes() {
    log "Deploying to Kubernetes..."
    
    # Create namespace
    kubectl create namespace streaming --dry-run=client -o yaml | kubectl apply -f -
    
    # Create secrets
    kubectl create secret generic streaming-secrets \
        --namespace streaming \
        --from-literal=database-password=$DB_PASSWORD \
        --from-literal=api-key=$API_KEY \
        --dry-run=client -o yaml | kubectl apply -f -
    
    # Deploy applications
    kubectl apply -f cloud/kubernetes/streaming-server/ -n streaming
    kubectl apply -f cloud/kubernetes/redis-cluster/ -n streaming
    kubectl apply -f cloud/kubernetes/monitoring/ -n streaming
    kubectl apply -f cloud/kubernetes/ingress/ -n streaming
    
    log "Kubernetes deployment completed"
}

wait_for_deployment() {
    log "Waiting for deployment to be ready..."
    
    # Wait for pods to be ready
    kubectl wait --namespace streaming \
        --for=condition=ready pod \
        --selector=app=streaming-server \
        --timeout=300s
    
    # Wait for load balancer
    local lb_hostname=""
    while [ -z "$lb_hostname" ]; do
        lb_hostname=$(kubectl get service streaming-service \
            --namespace streaming \
            -o jsonpath='{.status.loadBalancer.ingress[0].hostname}')
        sleep 10
    done
    
    log "Load Balancer URL: http://$lb_hostname"
}

run_tests() {
    log "Running deployment tests..."
    
    local lb_hostname=$(kubectl get service streaming-service \
        --namespace streaming \
        -o jsonpath='{.status.loadBalancer.ingress[0].hostname}')
    
    # Test HTTP endpoint
    curl -f http://$lb_hostname/health || error "Health check failed"
    
    # Test RTMP endpoint (basic connectivity)
    timeout 10 nc -z $lb_hostname 1935 || warn "RTMP port test failed"
    
    log "All tests passed"
}

enable_monitoring() {
    log "Setting up monitoring..."
    
    # Deploy Prometheus
    kubectl apply -f https://github.com/prometheus-operator/prometheus-operator/releases/latest/download/bundle.yaml
    
    # Deploy Grafana
    kubectl apply -f cloud/kubernetes/monitoring/grafana/
    
    # Set up alerts
    kubectl apply -f cloud/kubernetes/monitoring/alerts/
    
    log "Monitoring setup completed"
}

main() {
    log "Starting cloud deployment for Streaming Engine"
    
    check_prerequisites
    build_and_push_image
    deploy_infrastructure
    deploy_kubernetes
    wait_for_deployment
    run_tests
    enable_monitoring
    
    log "🎉 Cloud deployment completed successfully!"
    log "📊 Monitoring: http://grafana.streaming.svc.cluster.local"
    log "📈 Metrics: http://prometheus.streaming.svc.cluster.local"
    log "🎥 Streaming: rtmp://$(kubectl get service streaming-service -n streaming -o jsonpath='{.status.loadBalancer.ingress[0].hostname}')/live"
}

main "$@"