#!/usr/bin/env python3

import subprocess
import datetime


def get_git_commit():
    try:
        short = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode().strip()
        full = subprocess.check_output(['git', 'rev-parse', 'HEAD']).decode().strip()
        if subprocess.check_output(['git', 'status', '--porcelain']).strip():
            short += "-dirty"
            full += "-dirty"
        return short, full
    except:
        return "unknown", "unknown"

def generate_version_header():
    short_hash, full_hash = get_git_commit()

    # Check current version.h to avoid unnecessary rebuilds
    try:
        with open('src/version.h', 'r') as f:
            content = f.read()
            if f'#define GIT_COMMIT_HASH "{short_hash}"' in content and \
               f'#define GIT_COMMIT_FULL "{full_hash}"' in content:
                return short_hash # No changes, skip regeneration
    except FileNotFoundError:
        pass  # File doesn't exist, will create it

    timestamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    with open('src/version.h', 'w') as f:
        f.write(f'''#ifndef VERSION_H
    #define VERSION_H

    #define GIT_COMMIT_HASH "{short_hash}"
    #define GIT_COMMIT_FULL "{full_hash}"
    #define BUILD_TIMESTAMP "{timestamp}"

    #endif
    ''')
        
    return short_hash
        

short_hash = generate_version_header()
    
Import("env")

env.AddCustomTarget(
    name="publish_ota",
    title="Publish OTA",
    dependencies="$BUILD_DIR/${PROGNAME}.bin",
    actions=f"aws --profile eric s3 cp $SOURCE s3://enry-weather-nodes-fmw-update/{env['PIOENV']}-{short_hash}.bin --region eu-north-1",
    always_build=True,
)
