#!/usr/bin/env python3
import pathlib
import subprocess
import sys
import tempfile
import unittest


HERE = pathlib.Path(__file__).resolve().parent
SANITIZER = HERE / "QtSanitizeWebEngineNinja.py"


def write_archive(path, members):
    with path.open("wb") as archive:
        archive.write(b"!<arch>\n")
        for name, payload in members:
            encoded_name = (name + "/").encode("ascii")
            if len(encoded_name) > 16:
                raise ValueError(name)
            header = (
                encoded_name.ljust(16)
                + b"0".ljust(12)
                + b"0".ljust(6)
                + b"0".ljust(6)
                + b"100644".ljust(8)
                + str(len(payload)).encode("ascii").ljust(10)
                + b"`\n"
            )
            archive.write(header)
            archive.write(payload)
            if len(payload) % 2:
                archive.write(b"\n")


class SanitizerObjectArchiveTest(unittest.TestCase):
    def make_fixture(self, root, second_manifest_name="shared.o"):
        build_dir = root / "build"
        private_dir = root / "private"
        (build_dir / "obj").mkdir(parents=True)
        private_dir.mkdir()
        (private_dir / "qtwebengine-thirdparty-objects.rsp").write_text(
            "obj/one/shared.o\n"
            f"obj/two/{second_manifest_name}\n",
            encoding="utf-8",
        )
        (private_dir / "qtwebengine-thirdparty-archives.rsp").write_text(
            "/clang64/lib/qt6-webengine-private/obj/libdummy.a\n",
            encoding="utf-8",
        )
        write_archive(
            private_dir / "libQt6WebEngineThirdPartyObjects.a",
            [("shared.o", b"first"), ("shared.o", b"second")],
        )
        (build_dir / "obj" / "targets.ninja").write_text(
            "build obj/one/shared.o: cxx one.cc\n"
            f"build obj/two/{second_manifest_name}: cxx two.cc\n"
            "build obj/libdummy.a: alink obj/one/shared.o\n",
            encoding="utf-8",
        )
        return build_dir, private_dir

    def test_flattened_colliding_members_follow_manifest_order(self):
        with tempfile.TemporaryDirectory() as temp:
            build_dir, private_dir = self.make_fixture(pathlib.Path(temp))
            subprocess.run(
                [sys.executable, str(SANITIZER), str(build_dir), str(private_dir)],
                check=True,
                stdout=subprocess.PIPE,
                text=True,
            )
            self.assertEqual((build_dir / "obj/one/shared.o").read_bytes(), b"first")
            self.assertEqual((build_dir / "obj/two/shared.o").read_bytes(), b"second")
            self.assertFalse((build_dir / "shared.o").exists())
            ninja = (build_dir / "obj/targets.ninja").read_text(encoding="utf-8")
            self.assertIn("build obj/one/shared.o: phony", ninja)
            self.assertIn("build obj/two/shared.o: phony", ninja)

    def test_member_basename_must_match_manifest(self):
        with tempfile.TemporaryDirectory() as temp:
            build_dir, private_dir = self.make_fixture(
                pathlib.Path(temp), "different.o"
            )
            result = subprocess.run(
                [sys.executable, str(SANITIZER), str(build_dir), str(private_dir)],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("expected manifest object", result.stdout)


if __name__ == "__main__":
    unittest.main()
