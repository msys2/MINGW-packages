#!/usr/bin/env python3
"""Synthetic coverage for generated-action ordering repair."""
import contextlib
import io
import pathlib
import subprocess
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import QtRepairWebEngineNinja as repair
import QtWebEngineNinjaGraph as graph


SANITIZER = HERE / "QtSanitizeWebEngineNinja.py"


MANIFEST = r'''include_dirs = -Igen/.moc
rule action
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
  command = minify --in_folder gen/lit/bundled --in_files lit.rollup.js
rule cyclic
  command = generate --config gen/cyclic_alias
rule grd
  command = generate --manifest-files gen/lit/build_min_js_manifest.json
rule ts
  command = tsc --deps ../../third_party/polymer/tsconfig_library.json
rule cxx
  command = cxx $in
rule __third_party_blink_renderer_platform_loader_loader__jumbo_merge___build_toolchain_win_mingw_x64__rule
  command = merge $in
  rspfile_content = --inputs gen/blink/fetch_initiator_type_names.cc
rule link
  command = link $in
rule test_generator
  command = generate from-response-file

build gen/mojom/example.mojom-module: action source.mojom
build gen/mojom/example.mojom.h: mojom source.mojom
build gen/blink/protocol.json: action protocol_source.json
build gen/content/protocol.json: protocol gen_source.json
build gen/content/auction_protocol.json: protocol_config auction_source.json
build gen/lit/lit.js: action lit.ts
build gen/lit/bundled/lit.rollup.js: bundle lit.ts
build gen/lit/minified/lit.rollup.js gen/lit/build_min_js_manifest.json: minify lit.ts
build gen/lit/resources.grdp: grd lit.ts
build gen/cyclic/output.js: cyclic cyclic.ts
build gen/cyclic_alias: phony gen/cyclic/output.js
build gen/third_party/polymer/tsconfig_library.json: action polymer.ts
build gen/app/sub/tsconfig_build_ts.json: ts app.ts
build gen/loader_jumbo_9.cc: __third_party_blink_renderer_platform_loader_loader__jumbo_merge___build_toolchain_win_mingw_x64__rule
build gen/blink/fetch_initiator_type_names.cc: action names.in
rule __third_party_blink_public_test_mojom_automation__jumbo_merge___build_toolchain_win_mingw_x64__rule
  command = merge $in
  rspfile_content = --inputs gen/third_party/blink/public/test/mojom/automation.test-mojom.cc
build gen/third_party/blink/public/test/mojom/automation_jumbo_1.cc: __third_party_blink_public_test_mojom_automation__jumbo_merge___build_toolchain_win_mingw_x64__rule
build gen/third_party/blink/public/test/mojom/automation.test-mojom-module: action missing.test-mojom
build gen/third_party/blink/public/test/mojom/automation.test-mojom.cc: test_generator
build gen/.moc/location_provider_qt.moc: action location_provider_qt.cpp
build obj/core.o: cxx source.cc gen/loader_jumbo_9.cc
build QtWebEngineCore: link obj/core.o
'''


class GeneratedActionOrderingTest(unittest.TestCase):
    def run_repair(self, build_dir):
        (build_dir / "build.ninja").write_text(MANIFEST, encoding="utf-8")
        (build_dir / "source.cc").write_text(
            '#include "location_provider_qt.moc"\nint main() {}\n',
            encoding="utf-8",
        )
        (build_dir / "location_provider_qt.cpp").write_text(
            "class LocationProvider {};\n", encoding="utf-8")
        (build_dir / "names.in").write_text("names\n", encoding="utf-8")

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
            self.assertNotIn("gen/cyclic_alias",
                             by_output["gen/cyclic/output.js"].order_only)
            self.assertIn("gen/third_party/polymer/tsconfig_library.json",
                          by_output["gen/app/sub/tsconfig_build_ts.json"].order_only)
            self.assertIn("gen/blink/fetch_initiator_type_names.cc",
                          by_output["qtwebengine_generated_prerequisites"].inputs)
            self.assertIn("gen/.moc/location_provider_qt.moc",
                          by_output["qtwebengine_generated_prerequisites"].inputs)
            self.assertIn("gen/blink/fetch_initiator_type_names.cc",
                          by_output["gen/loader_jumbo_9.cc"].order_only)
            self.assertIn(
                "gen/third_party/blink/public/test/mojom/automation.test-mojom-module",
                by_output[
                    "gen/third_party/blink/public/test/mojom/automation.test-mojom.cc"
                ].order_only,
            )
            self.assertNotIn(
                "gen/third_party/blink/public/test/mojom/automation.test-mojom.cc",
                by_output[
                    "gen/third_party/blink/public/test/mojom/automation_jumbo_1.cc"
                ].order_only,
            )
            self.assertNotIn(
                "gen/third_party/blink/public/test/mojom/automation.test-mojom.cc",
                by_output["qtwebengine_generated_prerequisites"].inputs,
            )
            self.assertNotIn(
                "gen/third_party/blink/public/test/mojom/automation_jumbo_1.cc",
                by_output["qtwebengine_generated_prerequisites"].inputs,
            )
            self.assertIn("qtwebengine_generated_prerequisites",
                          by_output["obj/core.o"].order_only)

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


class PrebuiltGeneratedOutputTest(unittest.TestCase):
    def run_sanitizer(self, build_dir, private_dir, manifest):
        subprocess.run(
            [sys.executable, str(SANITIZER), str(build_dir), str(private_dir),
             str(manifest)],
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        )

    def test_phonies_complete_prebuilt_action_edge(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            build_dir = root / "build"
            private_dir = root / "private"
            build_dir.mkdir()
            private_dir.mkdir()
            (private_dir / "qtwebengine-thirdparty-archives.rsp").write_text(
                "/ucrt64/lib/qt6-webengine-private/obj/libsupplied.a\n",
                encoding="utf-8",
            )
            manifest = root / "prebuilt-gen.rsp"
            manifest.write_text(
                "gen/tsproto/third_party/perfetto/perfetto_config.ts\n"
                "gen/webui/resources.json\n",
                encoding="utf-8",
            )
            (build_dir / "build.ninja").write_text(r'''rule tsproto
  command = missing-ts-proto $in
rule action
  command = action $in
build gen/tsproto/third_party/perfetto/perfetto_config.ts: tsproto config.proto
build gen/webui/resources.json: action resources.grd
build gen/mixed.ts gen/unpackaged.ts: action input.idl
''', encoding="utf-8")

            self.run_sanitizer(build_dir, private_dir, manifest)
            edges, _, _ = graph.load(build_dir)
            by_output = {edge.outputs[0]: edge for edge in edges if edge.outputs}
            self.assertEqual("phony", by_output[
                "gen/tsproto/third_party/perfetto/perfetto_config.ts"].rule)
            self.assertEqual([], by_output[
                "gen/tsproto/third_party/perfetto/perfetto_config.ts"].all_inputs())
            self.assertEqual("phony", by_output["gen/webui/resources.json"].rule)
            self.assertEqual([], by_output["gen/webui/resources.json"].all_inputs())
            self.assertEqual("action", by_output["gen/mixed.ts"].rule)


if __name__ == "__main__":
    unittest.main()
