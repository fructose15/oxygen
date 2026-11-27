class Oxygen < Formula
  desc "English-syntax multithreaded programming language (.o2) by Fructose"
  homepage "https://github.com/fructose15/oxygen"
  head "https://github.com/fructose15/oxygen.git", branch: "main"
  license "MIT"

  def install
    system "chmod", "+x", "configure"
    system "./configure", "--prefix=#{prefix}"
    system "make"
    system "make", "install"
  end

  test do
    assert_match "Fructose", shell_output("#{bin}/oxygen --codeowner")
    assert_match "ok", shell_output("#{bin}/oxygen --command 'print text \"ok\"'")
  end
end
