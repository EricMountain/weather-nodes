#!/usr/bin/env python3

# import subprocess
import datetime

from git import get_git_commit


def generate_version_header():
    short_hash, full_hash = get_git_commit()

    # Check current version.h to avoid unnecessary rebuilds
    try:
        with open('lib/config/version.h', 'r') as f:
            content = f.read()
            if f'#define GIT_COMMIT_HASH "{short_hash}"' in content and \
               f'#define GIT_COMMIT_FULL "{full_hash}"' in content:
                return short_hash # No changes, skip regeneration
    except FileNotFoundError:
        pass  # File doesn't exist, will create it

    timestamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    with open('lib/config/version.h', 'w') as f:
        f.write(f'''#ifndef VERSION_H
#define VERSION_H

#define GIT_COMMIT_HASH "{short_hash}"
#define GIT_COMMIT_FULL "{full_hash}"
#define BUILD_TIMESTAMP "{timestamp}"

#endif
    ''')
        
    return short_hash
        

short_hash = generate_version_header()
