package containers

// ========== //
// == Misc == //
// ========== //

// InterfaceContainer interface
type InterfaceContainer interface {
	Init() error
	IsRunning() bool
	StopAllContainers() error
	StartAllContainers() error
	GetContainerInfo(string) (string, string, string)
	ForceStopContainer(string) error
	StartRoutine()
}
