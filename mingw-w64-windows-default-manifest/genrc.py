#!/usr/bin/env python3
"""Generate a Windows RT_MANIFEST resource (.rc) with minified XML."""

import re

XML_TEMPLATE = """\
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="asInvoker"/>
      </requestedPrivileges>
    </security>
  </trustInfo>
  <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
    <application>
      <!--The ID below indicates application support for Windows Vista -->
      <supportedOS Id="{e2011457-1546-43c5-a5fe-008deee3d3f0}"/>
      <!--The ID below indicates application support for Windows 7 -->
      <supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}"/>
      <!--The ID below indicates application support for Windows 8 -->
      <supportedOS Id="{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}"/>
      <!--The ID below indicates application support for Windows 8.1 -->
      <supportedOS Id="{1f676c76-80e1-4239-95bb-83d0f6d0da78}"/>
      <!--The ID below indicates application support for Windows 10 -->
      <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/>
    </application>
  </compatibility>
</assembly>
"""

def minify_xml(xml: str) -> str:
    """Strip comments and collapse whitespace between tags."""
    # Remove XML comments
    xml = re.sub(r"<!--.*?-->", "", xml, flags=re.DOTALL)
    # Collapse whitespace between tags
    xml = re.sub(r">\s+<", "><", xml)
    # Trim leading/trailing whitespace
    return xml.strip()


def escape_rc_string(s: str) -> str:
    """Escape a string for use inside an RC BEGIN/END block."""
    return s.replace('"', '""')


def generate_rc(xml: str, resource_id: int = 1, resource_type: int = 24) -> str:
    data = escape_rc_string(minify_xml(xml))
    return f"""\
LANGUAGE 0, 0

/* CREATEPROCESS_MANIFEST_RESOURCE_ID RT_MANIFEST MOVEABLE PURE DISCARDABLE */
{resource_id} {resource_type} MOVEABLE PURE DISCARDABLE
BEGIN
  "{data}"
END
"""


def main():
    import argparse

    parser = argparse.ArgumentParser(
        description="Generate a minified RT_MANIFEST .rc resource."
    )
    parser.add_argument(
        "-o", "--output", help="Output .rc file (defaults to stdout)."
    )
    args = parser.parse_args()

    rc = generate_rc(XML_TEMPLATE)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(rc)
    else:
        print(rc, end="")


if __name__ == "__main__":
    main()
