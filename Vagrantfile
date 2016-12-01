Vagrant.configure(2) do |config|
  config.vm.box = "box-cutter/ubuntu1604-desktop"

  config.vm.provider "virtualbox" do |v|
    v.gui = true
  end

  config.vm.provision "shell" do |s|
    s.inline = $provisioner
  end
end

$provisioner = <<SCRIPT
echo Updating package sources...
sudo apt update
echo Upgrading packages...
sudo apt full-upgrade -y
echo Installing `cmake`...
sudo apt install cmake -y
SCRIPT
