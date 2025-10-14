"""
HTML rendering utilities for the graphs lambda.
"""
from typing import Dict, List, Optional
import os


def load_static_file(filename: str) -> str:
    """Load content from a static file"""
    try:
        static_dir = os.path.join(os.path.dirname(__file__), 'static')
        file_path = os.path.join(static_dir, filename)
        with open(file_path, 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        # TODO: Log error and throw exception
        return ""


def load_template(template_name: str) -> str:
    """Load content from a template file"""
    try:
        template_dir = os.path.join(os.path.dirname(__file__), 'templates')
        template_path = os.path.join(template_dir, template_name)
        with open(template_path, 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        # TODO: Log error and throw exception
        return ""


def generate_device_checkboxes(available_devices: List[Dict[str, str]]) -> str:
    """Generate HTML for device checkboxes"""
    device_checkboxes = ""
    for i, device in enumerate(available_devices):
        checked = "checked" if i == 0 else ""  # Check first device by default
        device_checkboxes += f'''
                <div class="device-checkbox">
                    <input type="checkbox" name="device" value="{device['device_id']}" id="{device['device_id']}" {checked}>
                    <label for="{device['device_id']}">{device['display_name']}</label>
                </div>'''
    return device_checkboxes


def generate_html_interface(available_devices: Optional[List[Dict[str, str]]] = None) -> str:
    """Generate the HTML interface with interactive controls and D3.js graphs"""
    if not available_devices:
        available_devices = [{"device_id": "displaydev", "display_name": "Display Device"}]
    
    # Load static files
    css_content = load_static_file('styles.css')
    js_content = load_static_file('charts.js')
    
    # Generate device checkboxes HTML
    device_checkboxes = generate_device_checkboxes(available_devices)
    
    # Load and format template
    html_template = load_template('index.html')
    
    # Replace template placeholders (including those in comments)
    return html_template.replace(
        '/* {css_content} */', css_content
    ).replace(
        '/* {js_content} */', js_content
    ).replace(
        '{device_checkboxes}', device_checkboxes
    )