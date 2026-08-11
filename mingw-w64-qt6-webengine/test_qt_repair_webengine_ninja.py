#!/usr/bin/env python3
"""Synthetic coverage for generated-action ordering repair."""
import contextlib
import io
import pathlib
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import QtRepairWebEngineNinja as repair
import QtWebEngineNinjaGraph as graph


MANIFEST = r'''rule action
  command = action $in
rule mojom
  command = mojom_bindings_generator.py --filelist=gen/mojom/example.rsp
rule protocol
  command = concatenate gen/blink/protocol.json gen/content/protocol.json
rule protocol_config
  command = codegen --config_value protocol.path=gen/blink/protocol.json
rule bundle
  command = bundle --input gen/lit --js_module_in_files lit.js
rule minify
  command = minify --in_folder gen/lit/bundled
rule grd
  command = generate --manifest-files gen/lit/build_min_js_manifest.json
rule ts
  command = tsc --deps ../../third_party/polymer/tsconfig_library.json
rule cxx
  command = cxx $in
rule link
  command = link $in

build gen/mojom/example.mojom-module: action source.mojom
build gen/mojom/example.mojom.h: mojom source.mojom
build gen/blink/protocol.json: action protocol_source.json
build gen/content/protocol.json: protocol gen_source.json
build gen/content/auction_protocol.json: protocol_config auction_source.json
build gen/lit/lit.js: action lit.ts
build gen/lit/bundled/lit.rollup.js: bundle lit.ts
build gen/lit/minified/lit.rollup.js gen/lit/build_min_js_manifest.json: minify lit.ts
build gen/lit/resources.grdp: grd lit.ts
build gen/third_party/polymer/tsconfig_library.json: action polymer.ts
build gen/app/sub/tsconfig_build_ts.json: ts app.ts
build obj/core.o: cxx source.cc
build QtWebEngineCore: link obj/core.o
'''


class GeneratedActionOrderingTest(unittest.TestCase):
    def run_repair(self, build_dir):
        (build_dir / "build.ninja").write_text(MANIFEST, encoding="utf-8")
        (build_dir / "source.cc").write_text("int main() {}\n", encoding="utf-8")

        old_argv = sys.argv
        try:
            sys.argv = [repair.__file__, str(build_dir)]
            with contextlib.redirect_stdout(io.StringIO()):
                repair.main()
        finally:
            sys.argv = old_argv

    def test_recovers_every_generated_action_input_class(self):
        with tempfile.TemporaryDirectory() as temp:
            build_dir = pathlib.Path(temp)
            self.run_repair(build_dir)
            edges, _, _ = graph.load(build_dir)
            by_output = {edge.outputs[0]: edge for edge in edges if edge.outputs}

            self.assertIn("gen/mojom/example.mojom-module",
                          by_output["gen/mojom/example.mojom.h"].order_only)
            self.assertIn("gen/blink/protocol.json",
                          by_output["gen/content/protocol.json"].order_only)
            self.assertIn("gen/blink/protocol.json",
                          by_output["gen/content/auction_protocol.json"].order_only)
            self.assertIn("gen/lit/lit.js",
                          by_output["gen/lit/bundled/lit.rollup.js"].order_only)
            self.assertNotIn("gen/lit/minified/lit.rollup.js",
                             by_output["gen/lit/bundled/lit.rollup.js"].order_only)
            self.assertIn("gen/lit/bundled/lit.rollup.js",
                          by_output["gen/lit/minified/lit.rollup.js"].order_only)
            self.assertIn("gen/lit/build_min_js_manifest.json",
                          by_output["gen/lit/resources.grdp"].order_only)
            self.assertIn("gen/third_party/polymer/tsconfig_library.json",
                          by_output["gen/app/sub/tsconfig_build_ts.json"].order_only)

    def test_repair_is_idempotent(self):
        with tempfile.TemporaryDirectory() as temp:
            build_dir = pathlib.Path(temp)
            self.run_repair(build_dir)
            first = (build_dir / "build.ninja").read_text(encoding="utf-8")
            old_argv = sys.argv
            try:
                sys.argv = [repair.__file__, str(build_dir)]
                with contextlib.redirect_stdout(io.StringIO()):
                    repair.main()
            finally:
                sys.argv = old_argv
            self.assertEqual(first, (build_dir / "build.ninja").read_text(
                encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
