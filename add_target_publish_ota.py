#!/usr/bin/env python3

from git import get_git_commit

Import("env")

short_hash, _ = get_git_commit()

env.AddCustomTarget(
    name="publish_ota",
    title="Publish OTA",
    dependencies="$BUILD_DIR/${PROGNAME}.bin",
    actions=[
        f"aws --profile eric s3 cp $SOURCE s3://enry-weather-nodes-fmw-update/{env['PIOENV']}-{short_hash}.bin --region eu-north-1",
        f"echo 'target_version = {short_hash}'",
        f"echo -n {short_hash} | xsel -b"
    ],
    always_build=True,
)
