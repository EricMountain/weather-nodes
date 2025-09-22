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

provider "aws" {
  region = "eu-north-1"
  profile = "eric"
}

provider "aws" {
  alias  = "paris"
  region = "eu-west-3"
  profile = "eric"
}
