# cloud/terraform/aws/main.tf
terraform {
  required_version = ">= 1.0"
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 4.0"
    }
    kubernetes = {
      source  = "hashicorp/kubernetes"
      version = "~> 2.0"
    }
  }

  backend "s3" {
    bucket = "streaming-engine-tfstate"
    key    = "production/terraform.tfstate"
    region = "us-west-2"
    encrypt = true
    dynamodb_table = "terraform-locks"
  }
}

provider "aws" {
  region = var.aws_region
  default_tags {
    tags = {
      Project     = "streaming-engine"
      Environment = var.environment
      ManagedBy   = "terraform"
    }
  }
}

# VPC Module
module "vpc" {
  source = "./modules/vpc"

  name                 = "streaming-${var.environment}"
  cidr                 = var.vpc_cidr
  azs                  = var.availability_zones
  private_subnets      = var.private_subnet_cidrs
  public_subnets       = var.public_subnet_cidrs
  enable_nat_gateway   = true
  single_nat_gateway   = false
  enable_dns_hostnames = true

  tags = {
    Environment = var.environment
  }
}

# EKS Cluster Module
module "eks" {
  source = "./modules/eks"

  cluster_name    = "streaming-${var.environment}"
  cluster_version = "1.27"

  vpc_id          = module.vpc.vpc_id
  subnet_ids      = module.vpc.private_subnets

  node_groups = {
    streaming-nodes = {
      desired_capacity = 3
      max_capacity     = 10
      min_capacity     = 1

      instance_types = ["c5.2xlarge", "c5.4xlarge"]
      capacity_type  = "SPOT"

      k8s_labels = {
        Environment = var.environment
        NodeGroup   = "streaming-nodes"
      }

      taints = [
        {
          key    = "streaming"
          value  = "true"
          effect = "NO_SCHEDULE"
        }
      ]
    }

    monitoring-nodes = {
      desired_capacity = 1
      max_capacity     = 3
      min_capacity     = 1

      instance_types = ["m5.large"]
      capacity_type  = "ON_DEMAND"

      k8s_labels = {
        Environment = var.environment
        NodeGroup   = "monitoring-nodes"
      }
    }
  }

  tags = {
    Environment = var.environment
  }
}

# RDS PostgreSQL for metadata
module "rds" {
  source = "./modules/rds"

  identifier = "streaming-${var.environment}"

  engine               = "postgres"
  engine_version       = "14.7"
  instance_class       = "db.r5.large"
  allocated_storage    = 100
  max_allocated_storage = 500

  db_name  = "streaming"
  username = var.db_username
  password = var.db_password
  port     = 5432

  vpc_security_group_ids = [module.eks.cluster_primary_security_group_id]
  subnet_ids            = module.vpc.private_subnets

  backup_retention_period = 7
  backup_window           = "03:00-04:00"
  maintenance_window      = "sun:04:00-sun:05:00"

  tags = {
    Environment = var.environment
  }
}

# S3 for video storage
resource "aws_s3_bucket" "video_storage" {
  bucket = "streaming-videos-${var.environment}-${random_id.bucket_suffix.hex}"

  tags = {
    Environment = var.environment
  }
}

resource "aws_s3_bucket_versioning" "video_storage" {
  bucket = aws_s3_bucket.video_storage.id
  versioning_configuration {
    status = "Enabled"
  }
}

resource "aws_s3_bucket_lifecycle_configuration" "video_storage" {
  bucket = aws_s3_bucket.video_storage.id

  rule {
    id     = "auto-delete-old-versions"
    status = "Enabled"

    noncurrent_version_expiration {
      noncurrent_days = 30
    }

    abort_incomplete_multipart_upload {
      days_after_initiation = 1
    }
  }
}

# CloudFront CDN for global delivery
resource "aws_cloudfront_distribution" "streaming_cdn" {
  origin {
    domain_name = aws_s3_bucket.video_storage.bucket_regional_domain_name
    origin_id   = "s3-video-storage"

    s3_origin_config {
      origin_access_identity = aws_cloudfront_origin_access_identity.streaming_oai.cloudfront_access_identity_path
    }
  }

  enabled             = true
  is_ipv6_enabled     = true
  comment             = "Streaming Engine CDN"

  default_cache_behavior {
    allowed_methods  = ["DELETE", "GET", "HEAD", "OPTIONS", "PATCH", "POST", "PUT"]
    cached_methods   = ["GET", "HEAD"]
    target_origin_id = "s3-video-storage"

    forwarded_values {
      query_string = false
      cookies {
        forward = "none"
      }
    }

    viewer_protocol_policy = "redirect-to-https"
    min_ttl                = 0
    default_ttl            = 3600
    max_ttl                = 86400

    # Lambda@Edge for token validation
    lambda_function_association {
      event_type   = "viewer-request"
      lambda_arn   = aws_lambda_function.token_validator.qualified_arn
      include_body = false
    }
  }

  price_class = "PriceClass_All"

  restrictions {
    geo_restriction {
      restriction_type = "none"
    }
  }

  viewer_certificate {
    cloudfront_default_certificate = true
    ssl_support_method             = "sni-only"
  }

  tags = {
    Environment = var.environment
  }
}