class Vframetest < Formula
  desc "Professional media frame I/O benchmark and testing tool"
  homepage "https://github.com/ssotoa70/vframetest"
  url "https://github.com/ssotoa70/vframetest/archive/refs/tags/v3025.10.2.tar.gz"
  sha256 "ce5a35cc0cec5fdc3de32615fd3aa2769ca11c7aa9ff534d33a2205fbe573f94"
  license "GPL-2.0-or-later"

  depends_on "make" => :build

  def install
    system "make", "clean"
    system "make"
    bin.install "build/vframetest"
  end

  test do
    assert_match "vframetest 3025.10.2", shell_output("#{bin}/vframetest --version")
  end
end
