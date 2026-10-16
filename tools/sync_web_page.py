with open('portal_page.html', 'r', encoding='utf-8') as f:
    html = f.read()

prefix = '#pragma once\n#include <Arduino.h>\n\n// Rick & Morty Portal Gun Captive Portal Web Page\n// Embedded directly in PROGMEM flash\nstatic const char PAGE_INDEX[] PROGMEM = R"rawliteral('
suffix = ')rawliteral";\n'

with open('include/web_page.h', 'w', encoding='utf-8') as f:
    f.write(prefix + html + suffix)

print(f'Successfully updated include/web_page.h ({len(html)} bytes)')

