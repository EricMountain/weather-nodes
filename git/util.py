import subprocess

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
