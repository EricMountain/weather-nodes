terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
    }
    archive = {
      source  = "hashicorp/archive"
    }
  }
  required_version = ">= 0.13"

  backend "local" {
    path = ".terraform.tfstate"
  }
}

# Configure the AWS Provider
provider "aws" {
  region = "eu-west-3"
  profile = "eric"
}
