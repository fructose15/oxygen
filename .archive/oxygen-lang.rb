class OxygenLang < Formula
  desc "English-syntax multithreaded programming language"
  homepage "https://github.com/fructose15/oxygen"
  url "https://codeload.github.com/fructose15/oxygen/tar.gz/refs/tags/OxygenSource"
  version "1.0.0"
  sha256 "e098b3317aabe0cd5db87a39b3f1ee8715e93117a280db78d3e9fa65f523d65b"
  license "MIT"

  def install
    build_dir = File.exist?("configure") ? "." : "oxygen-lang"
    cd build_dir do
      system "chmod", "+x", "configure"
      system "./configure", "--prefix=#{prefix}"
      system "make"
      system "make", "install"
    end
  end

  test do
    assert_match "Fructose", shell_output("#{bin}/oxygen --codeowner")
    assert_match "ok", shell_output("#{bin}/oxygen --command 'print text \"ok\"'")
  end
end
